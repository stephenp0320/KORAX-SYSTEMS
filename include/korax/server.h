/* Unix-domain socket NDJSON feed: one listening socket, many subscribed
 * clients, all fed the same line-delimited JSON stream (events "hello",
 * "track_update", "track_expired" -- see output_json.h for the per-track
 * payload contract). Downstream consumers parse by newline; every write
 * here must stay one complete JSON object per line. */
#ifndef KORAX_SERVER_H
#define KORAX_SERVER_H
#include <stddef.h>
#include "korax/rook_track.h"
#define KX_DEFAULT_SOCKET "/run/korax/korax.sock"
#define KX_MAX_CLIENTS    32     /* bounded: refuse connections past this  */
#define KX_LISTEN_BACKLOG 8
#define BUFFER_SPACE 100
#define SOCKET_ERR -1
#define KX_DEFAULT_SOURCE_PATH "frames.txt"
#define ACTIVE_SOCKETS_REMAIN -1

typedef struct {
    int    listen_fd;                 /* listening socket, -1 if not open   */
    int    client_fds[KX_MAX_CLIENTS];/* connected subscribers              */
    size_t client_count;
    char   socket_path[108];
} server_t;

/* Binds and listens on socket_path (removing any stale socket file first);
 * puts the listening fd in non-blocking mode. Returns 0 on success,
 * SOCKET_ERR on any setup failure (bind/listen/fcntl). */
int server_init(server_t *server, const char *socket_path);
/* Accepts at most one pending connection per call (non-blocking accept),
 * sends "hello" then a "track_update" backfill of every current track.
 * Returns 0 whether or not a client was accepted/dropped, SOCKET_ERR only
 * for a hard accept()/track-table error -- a slow or gone client is not
 * an error, it's just dropped. */
int server_accept(server_t *server, const track_table_t *tracks);
/* Writes line to every connected client; a client whose write fails is
 * closed and removed (order in client_fds is not preserved -- see
 * remove_client's swap-with-last). Returns the count of clients the line
 * was actually delivered to, or -1 for bad args. */
int server_broadcast(server_t *server, const char *line, size_t len);
void server_drop_client(server_t *server, size_t index);
void server_close(server_t *server);

#endif
