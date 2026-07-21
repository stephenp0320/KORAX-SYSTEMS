#include <stdio.h>
#include "daemon/config.h"
#include <string.h>
#include "korax/server.h"
#define CONFIG_ERR -1

int config_defaults(korax_config_t *cfg){
    if (cfg == NULL){
        return CONFIG_ERR;
    }
    char *socket = strncpy(cfg->socket_path, KX_DEFAULT_SOCKET, sizeof cfg->socket_path - 1);
    if (!socket){
        return SOCKET_ERR;
    }

    return 0;
}
