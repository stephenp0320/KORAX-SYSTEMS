#include <stdio.h>
#include "daemon/config.h"
#include <string.h>
#include "korax/server.h"
#define CONFIG_ERR -1
#define CONFIG_OK 0

int config_defaults(korax_config_t *cfg) {
    if (cfg == NULL) {
        return CONFIG_ERR;
    }

    snprintf(cfg->socket_path, sizeof(cfg->socket_path), "%s", KX_DEFAULT_SOCKET);
    snprintf(cfg->source_path, sizeof(cfg->source_path), "%s", KX_DEFAULT_SOURCE_PATH);
    cfg->expiry_seconds = EXPIRY_SECONDS;
    cfg->grace_seconds = KX_DEFAULT_GRACE;
    cfg->max_speed_kmh = KX_DEFAULT_MAX_SPEED;
    cfg->log_level = KX_DEFAULT_LOG_LEVEL;

    if (cfg->expiry_seconds <= 0) {
        return CONFIG_ERR;
    }
    return CONFIG_OK;
}
