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

int set_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL failed");
        return -1;
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
        fprintf(stderr, "Failed to create socket, err=%d\n", errno);
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);

    if (bind(server->listen_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1){
        fprintf(stderr, "Failed to bind, err=%d\n", errno);
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    if (listen(server->listen_fd, KX_LISTEN_BACKLOG) == -1){
        fprintf(stderr, "Failed to listen err=%d\n", errno);
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    int non_blocking = set_socket_nonblocking(server->listen_fd);

    if (non_blocking == -1){
        fprintf(stderr, "set_socket_nonblocking err=%d\n", errno);
        close(server->listen_fd);
        return SOCKET_ERR;
    }

    server->client_count = 0;
    return 0;
}



int socket_setup(char *buffer){
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1){
        fprintf(stderr, "Failed to create socket, err=%d\n", errno);
        exit(4);
    }
    struct sockaddr_un addr;
    if (strlen(SOCKET_FILE) > sizeof(addr.sun_path) - 1){
        fprintf(stderr, "Socket name too long, socket sun path is size %zu\n", sizeof(addr.sun_path) - 1);
        exit(1);
    }
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_FILE, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "Failed to bind, err=%d\n", errno);
        exit(3);
    }
    if (listen(sfd, 5) == -1){
        fprintf(stderr, "Failed to listen err=%d\n", errno);
        exit(2);
    }
    while (1) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd == -1) {
            fprintf(stderr, "Cannot accept, err=%d\n", errno);
            exit(5);
        }
        int numRead;
        while ((numRead = read(cfd, buffer, BUFFER_SPACE)) > 0) {
            if (write(STDOUT_FILENO, buffer, numRead) != numRead) {
                fprintf(stderr, "Cannot write to stdout, err=%d\n", errno);
                exit(6);
            }
        }
        if (numRead == -1) {
            fprintf(stderr, "Error while reading, err=%d\n", errno);
            exit(8);
        }
        if (close(cfd) == -1) {
            fprintf(stderr, "Cannot close connected fd, err=%d\n", errno);
            exit(9);
        }
    }
    return 0;
}
