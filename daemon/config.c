#include <stdio.h>
#include "daemon/config.h"
#include <string.h>
#include "korax/server.h"
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include "daemon/log.h"
#define CONFIG_ERR -1
#define CONFIG_OK 0

int config_defaults(korax_config_t *cfg) {
    if (cfg == NULL) return CONFIG_ERR;

    snprintf(cfg->socket_path, sizeof(cfg->socket_path), "%s", KX_DEFAULT_SOCKET);
    snprintf(cfg->source_path, sizeof(cfg->source_path), "%s", KX_DEFAULT_SOURCE_PATH);
    cfg->expiry_seconds = EXPIRY_SECONDS;
    cfg->grace_seconds = KX_DEFAULT_GRACE;
    cfg->max_speed_kmh = KX_DEFAULT_MAX_SPEED;
    cfg->log_level = KX_DEFAULT_LOG_LEVEL;
    cfg->mqtt_enabled = KX_DEFAULT_MQTT_ENABLED;
    snprintf(cfg->mqtt_host,  sizeof cfg->mqtt_host,  "%s", KX_DEFAULT_MQTT_HOST);
    cfg->mqtt_port    = KX_DEFAULT_MQTT_PORT;
    snprintf(cfg->mqtt_topic, sizeof cfg->mqtt_topic, "%s", KX_DEFAULT_MQTT_TOPIC);

    if (cfg->expiry_seconds <= 0) return CONFIG_ERR;
    return CONFIG_OK;
}

int config_load_file(korax_config_t *cfg, const char *path) {
    if (cfg == NULL || path == NULL) return CONFIG_ERR;

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        if (errno == ENOENT) return CONFIG_OK;
        log_msg(LOG_ERROR, "Failed to open the configuration file '%s': %s", path, strerror(errno));
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
                log_msg(LOG_ERROR, "syntax error on line %d: Malformed section header", line_counter);
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
            char *endptr;
            if (strcmp(section, "output") == 0 && strcmp(key, "socket_path") == 0) {
                strncpy(cfg->socket_path, value, sizeof(cfg->socket_path) - 1);
                cfg->socket_path[sizeof(cfg->socket_path) - 1] = '\0';

            } else if (strcmp(section, "capture") == 0 && strcmp(key, "source") == 0) {
                strncpy(cfg->source_path, value, sizeof(cfg->source_path) - 1);
                cfg->source_path[sizeof(cfg->source_path) - 1] = '\0';

            } else if (strcmp(section, "tracks") == 0 && strcmp(key, "expiry_seconds") == 0) {
                long res = strtol(value, &endptr , 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                cfg->expiry_seconds = (int)res;

            } else if (strcmp(section, "validator") == 0 && strcmp(key, "grace_seconds") == 0) {
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                cfg->grace_seconds = (int)res;

            } else if (strcmp(section, "validator") == 0 && strcmp(key, "max_speed_kmh") == 0) {
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                cfg->max_speed_kmh = res;

            } else if (strcmp(section, "log") == 0 && strcmp(key, "level") == 0) {
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                cfg->log_level = res;

            } else {
                log_msg(LOG_ERROR, "Error line %d: Unknown key '%s' in section '[%s]'", line_counter, key, section);
                fclose(fp);
                return CONFIG_ERR;
            }
        } else {
            log_msg(LOG_ERROR, "Syntax error line %d: Invalid line format", line_counter);
            fclose(fp);
            return CONFIG_ERR;
        }
    }
    fclose(fp);
    return CONFIG_OK;
}

int config_apply_cli(korax_config_t *cfg, int argc, char **argv){
    static struct option opts[] = {
        {"socket",    required_argument, 0, 's'},
        {"source",    required_argument, 0, 'f'},
        {"expiry",    required_argument, 0, 'e'},
        {"grace",     required_argument, 0, 'g'},
        {"max-speed", required_argument, 0, 'm'},
        {"log-level", required_argument, 0, 'l'},
        {"config",    required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "s:f:e:g:m:l:c:", opts, NULL)) != -1){
        switch (c){
            case 's':
                strncpy(cfg->socket_path, optarg, sizeof cfg->socket_path - 1);
                cfg->socket_path[sizeof cfg->socket_path - 1] = '\0';
                break;
            case 'f':
                strncpy(cfg->source_path, optarg, sizeof cfg->source_path - 1);
                cfg->source_path[sizeof cfg->source_path - 1] = '\0';
                break;
            case 'e':
                cfg->expiry_seconds = (int)strtol(optarg, NULL, 10);
                break;
            case 'g':
                cfg->grace_seconds = (int)strtol(optarg, NULL, 10);
                break;
            case 'm':
                cfg->max_speed_kmh = strtod(optarg, NULL);
                break;
            case 'l':
                cfg->log_level = (int)strtol(optarg, NULL, 10);
                break;
            case 'c':
                break;
            default:
                return -1;
        }
    }
    return 0;
}
