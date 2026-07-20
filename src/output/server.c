#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

const char* SOCKET_FILE = "/run/korax/korax.socket";
#define BUFFER_SPACE 100
