#include <stdio.h>
#include "daemon/config.h"
#include <string.h>
#include "korax/server.h"
#include <errno.h>
#include <stdlib.h>
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

int config_load_file(korax_config_t *cfg, const char *path) {
    if (cfg == NULL || path == NULL) {
        return CONFIG_ERR;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return CONFIG_OK;
        }
        fprintf(stderr, "Failed to open config file '%s': %s\n", path, strerror(errno));
        return CONFIG_ERR;
    }

    char section[32] = "";
    char line[256];
    int line_counter = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_counter++;

        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '#' || *p == ';' || *p == '\n' || *p == '\r' || *p == '\0') {
            continue;
        }

        if (*p == '[') {
            p++;
            size_t current_size = 0;
            while (*p != ']' && *p != '\0' && *p != '\n' && *p != '\r' && current_size < sizeof(section) - 1) {
                section[current_size++] = *p++;
            }
            section[current_size] = '\0';

            if (*p != ']') {
                fprintf(stderr, "Syntax error line %d: Malformed section header\n", line_counter);
                fclose(fp);
                return CONFIG_ERR;
            }
            continue;
        }

        char *eq = strchr(p, '=');
        if (eq != NULL) {
            *eq = '\0';

            char *key = p;
            char *value = eq + 1;
            char *kend = eq - 1;
            while (kend >= key && (*kend == ' ' || *kend == '\t')) {
                *kend-- = '\0';
            }

            while (*value == ' ' || *value == '\t') value++;

            char *vend = value + strlen(value) - 1;
            while (vend >= value && (*vend == ' ' || *vend == '\t' || *vend == '\n' || *vend == '\r')) {
                *vend-- = '\0';
            }

            if (strcmp(section, "output") == 0 && strcmp(key, "socket_path") == 0) {
                strncpy(cfg->socket_path, value, sizeof(cfg->socket_path) - 1);
                cfg->socket_path[sizeof(cfg->socket_path) - 1] = '\0';
            } else if (strcmp(section, "capture") == 0 && strcmp(key, "source") == 0) {
                strncpy(cfg->source_path, value, sizeof(cfg->source_path) - 1);
                cfg->source_path[sizeof(cfg->source_path) - 1] = '\0';
            } else if (strcmp(section, "tracks") == 0 && strcmp(key, "expiry_seconds") == 0) {
                cfg->expiry_seconds = atoi(value);
            } else if (strcmp(section, "validator") == 0 && strcmp(key, "grace_seconds") == 0) {
                cfg->grace_seconds = atoi(value);
            } else if (strcmp(section, "validator") == 0 && strcmp(key, "max_speed_kmh") == 0) {
                cfg->max_speed_kmh = atof(value);
            } else if (strcmp(section, "log") == 0 && strcmp(key, "level") == 0) {
                cfg->log_level = atoi(value);
            } else {
                fprintf(stderr, "Error line %d: Unknown key '%s' in section '[%s]'\n", line_counter, key, section);
                fclose(fp);
                return CONFIG_ERR;
            }
        } else {
            fprintf(stderr, "Syntax error line %d: Invalid line format\n", line_counter);
            fclose(fp);
            return CONFIG_ERR;
        }
    }
    fclose(fp);
    return CONFIG_OK;
}
