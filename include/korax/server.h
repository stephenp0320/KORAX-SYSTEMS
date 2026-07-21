#ifndef KORAX_SERVER_H
#define KORAX_SERVER_H
#include <stddef.h>
#include "korax/rook_track.h"
#define KX_DEFAULT_SOCKET "/run/korax/korax.sock"
#define KX_MAX_CLIENTS    32     /* bounded: refuse connections past this  */
#define KX_LISTEN_BACKLOG 8
#define BUFFER_SPACE 100
#define SOCKET_ERR -1

typedef struct {
    int    listen_fd;                 /* listening socket, -1 if not open   */
    int    client_fds[KX_MAX_CLIENTS];/* connected subscribers              */
    size_t client_count;
    char   socket_path[108];
} server_t;

int server_init(server_t *server, const char *socket_path);
int server_accept(server_t *server, const track_table_t *tracks);
int server_broadcast(server_t *server, const char *line, size_t len);
void server_close(server_t *server);

#endif
