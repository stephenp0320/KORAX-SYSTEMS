/*
 * The output stage's transport: a Unix-domain socket serving NDJSON to any
 * number of subscribers (hello / track_update / track_expired events, one
 * JSON object per line). The line framing and per-track JSON shape are a
 * public contract shared with output_json.c/output_mqtt.c -- consumers
 * split on '\n' and expect a complete object each time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "korax/server.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "korax/output_json.h"
#include <signal.h>

int set_socket_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1){
        return -1;
    }
    return 0;
}

/*
 * Blocking (retries on EINTR) full write of one already-framed line; the
 * fd itself may be non-blocking, so a would-block short write is reported
 * as failure (-1) rather than looped on, so callers can drop a stalled
 * client instead of hanging the whole broadcast.
 */
static int send_line(int fd, const char *buf, size_t len){
    size_t sent = 0;
    while (sent < len){
        ssize_t n = write(fd, buf + sent, len - sent);
        if (n > 0){
            sent += (size_t)n;
        } else if (n == -1 && errno == EINTR){
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

int server_init(server_t *server, const char *socket_path){
    if (server == NULL || socket_path == NULL){
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);
    server->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server->listen_fd == -1){
        return SOCKET_ERR;
    }
    struct sockaddr_un addr;

    if (strlen(socket_path) > sizeof(addr.sun_path) - 1){
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);
    snprintf(server->socket_path, sizeof(server->socket_path), "%s", socket_path);
    if (bind(server->listen_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1){
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    if (listen(server->listen_fd, KX_LISTEN_BACKLOG) == -1){
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    int non_blocking = set_socket_nonblocking(server->listen_fd);

    if (non_blocking == -1){
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    server->client_count = 0;
    return 0;
}

int server_accept(server_t *server, const track_table_t *track){
    if (server == NULL || track == NULL){
        return SOCKET_ERR;
    }

    int clientfd = accept(server->listen_fd, NULL, NULL);
     if (clientfd == -1){
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return SOCKET_ERR;
    }

    if (server->client_count >= KX_MAX_CLIENTS){
        close(clientfd);
        return 0;
    }
    int non_blocking = set_socket_nonblocking(clientfd);

    if (non_blocking == -1){
        close(clientfd);
        return SOCKET_ERR;
    }
    char line[640];
    server->client_fds[server->client_count++] = clientfd;
    int len = snprintf(line, sizeof line, "{\"event\":\"hello\",\"schema_version\":%d}\n", KX_JSON_SCHEMA_VERSION);

    if (len < 0 || (size_t)len >= sizeof(line) || send_line(clientfd, line, (size_t)len) == -1){
        close(clientfd);
        server->client_count--;
        return 0;
    }

    for (int i = 0; i < track->num_tracks; i++){
        /* buffer/line must stay large enough for a track's full findings[]
         * (see FBUF_SIZE in output_json.c); too small makes track_to_json
         * return -2, and this backfill silently skips that track instead
         * of erroring -- a newly-connected client just won't see it. */
        char buffer[512];
        if (track_to_json(&track->tracks[i], buffer, sizeof(buffer)) < 0){
            continue;
        }
        len = snprintf(line, sizeof line, "{\"event\":\"track_update\",\"track\":%s}\n", buffer);

        if (len < 0 || (size_t)len >= sizeof(line)){
            continue;
        }

        if (send_line(clientfd, line, (size_t)len) == -1){
            close(clientfd);
            server->client_count--;
            return 0;
        }
    }
    return 0;
}

/* O(1) removal: swaps the last fd into slot i, so client_fds order is not
 * preserved -- fine, since indices are only ever used within one loop
 * iteration and never persisted. */
static void remove_client(server_t *server, size_t i){
    server->client_fds[i] = server->client_fds[server->client_count - 1];
    server->client_count--;
}

void server_drop_client(server_t *server, size_t index){
    if (server == NULL || index >= server->client_count){
        return;
    }
    close(server->client_fds[index]);
    remove_client(server, index);
}

int server_broadcast(server_t *server, const char *line, size_t len){
    if (server == NULL || line == NULL){
        return -1;
    }
    int delivered = 0;
    for (size_t i = 0; i < server->client_count; ){
        if (send_line(server->client_fds[i], line, len) == 0){
            delivered++;
            i++;
        } else {
            /* remove_client swaps the next client into slot i, so i must
             * NOT be incremented here or that swapped-in client gets
             * skipped this pass. */
            close(server->client_fds[i]);
            remove_client(server, i);
        }
    }
    return delivered;
}

void server_close(server_t *server){
    if (server == NULL) return;
    if (server->listen_fd >= 0){
        close(server->listen_fd);
        server->listen_fd = -1;
    }

    for (size_t i = 0; i < server->client_count; i++){
        close(server->client_fds[i]);
    }
    server->client_count = 0;
    unlink(server->socket_path);
}
