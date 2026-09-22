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
#define APPLY_CONFIG_CLI_COMPLETE 0

int config_defaults(korax_config_t *cfg) {
    if (cfg == NULL) return CONFIG_ERR;

    snprintf(cfg->socket_path, sizeof(cfg->socket_path), "%s", KX_DEFAULT_SOCKET);
    snprintf(cfg->source_path, sizeof(cfg->source_path), "%s", KX_DEFAULT_SOURCE_PATH);
    snprintf(cfg->track_export_path, sizeof(cfg->track_export_path), "%s", KX_DEFAULT_EXPORT_PATH);
    cfg->expiry_seconds = EXPIRY_SECONDS;
    cfg->log_level = KX_DEFAULT_LOG_LEVEL;
    cfg->thresholds = MAGPIE_DEFAULT_THRESHOLDS;
    cfg->mqtt_enabled = KX_DEFAULT_MQTT_ENABLED;
    snprintf(cfg->mqtt_host, sizeof cfg->mqtt_host,  "%s", KX_DEFAULT_MQTT_HOST);
    cfg->mqtt_port = KX_DEFAULT_MQTT_PORT;
    cfg->mqtt_qos = KX_DEFAULT_MQTT_QOS;
    cfg->mqtt_retain = KX_DEFAULT_MQTT_RETAIN;
    snprintf(cfg->mqtt_topic, sizeof cfg->mqtt_topic, "%s", KX_DEFAULT_MQTT_TOPIC);
    cfg->mqtt_tls_ca_path[0] = '\0';
    cfg->mqtt_tls_cert_path[0] = '\0';
    cfg->mqtt_tls_key_path[0] = '\0';
    cfg->mqtt_username[0] = '\0';
    cfg->mqtt_password[0] = '\0';
    snprintf(cfg->mqtt_creds_path, sizeof cfg->mqtt_creds_path, "%s", KX_DEFAULT_MQTT_CREDS_PATH);
    if (cfg->expiry_seconds <= 0)return CONFIG_ERR;
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

        if (*p == '#' || *p == ';' || *p == '\n' || *p == '\r' || *p == '\0') continue;
        if (*p == '[') {
            p++;
            size_t current_size = 0;
            while (*p != ']' && *p != '\0' && *p != '\n' && *p != '\r' && current_size < sizeof(section) - 1) section[current_size++] = *p++;
    
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

            while (kend >= key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';
            while (*value == ' ' || *value == '\t') value++;
            
            char *vend = value + strlen(value) - 1;
            while (vend >= value && (*vend == ' ' || *vend == '\t' || *vend == '\n' || *vend == '\r')) *vend-- = '\0';
            
            char *endptr;
            if (strcmp(section, OUTPUT_SECTION) == 0 && strcmp(key, "socket_path") == 0) {
                strncpy(cfg->socket_path, value, sizeof(cfg->socket_path) - 1);
                cfg->socket_path[sizeof(cfg->socket_path) - 1] = '\0';

            } else if (strcmp(section, CAPTURE_SECTION) == 0 && strcmp(key, "source") == 0) {
                strncpy(cfg->source_path, value, sizeof(cfg->source_path) - 1);
                cfg->source_path[sizeof(cfg->source_path) - 1] = '\0';

            } else if (strcmp(section, TRACKS_SECTION) == 0 && strcmp(key, "expiry_seconds") == 0) {
                long res = strtol(value, &endptr , 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < 0 || res > KX_MAX_EXPIRY_SECONDS) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and 300.", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->expiry_seconds = (int)res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "grace_seconds") == 0) {
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < 0 || res > KX_MAX_GRACE_SECONDS) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %d.", line_counter, value, key, section, KX_MAX_GRACE_SECONDS);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.grace_seconds = (int)res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "max_speed_kmh") == 0) {
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > MAX_SPEED_KMH){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %.2f.", line_counter, value, key, section, MAX_SPEED_KMH);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.max_speed_kmh = res;

            } else if (strcmp(section, LOG_SECTION) == 0 && strcmp(key, "level") == 0) {
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < MIN_LOG_LEVEL || res > MAX_LOG_LEVEL) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between %d and %d.", line_counter, value, key, section, MIN_LOG_LEVEL, MAX_LOG_LEVEL);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->log_level = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "id_churn_limit") == 0){
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > KX_ID_CHURN_LIMIT){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %d.", line_counter, value, key, section, KX_ID_CHURN_LIMIT);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.id_churn_limit = (int)res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "teleport_threshold") == 0){
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > KX_TELEPORT_THRESHOLD){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %.2f.", line_counter, value, key, section, KX_TELEPORT_THRESHOLD);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.teleport_threshold = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 &&  strcmp(key, "min_broadcast_hz") == 0){
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < KX_MIN_BROADCAST_RATE){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be >= %.2f.", line_counter, value, key, section, KX_MIN_BROADCAST_RATE);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.min_broadcast_hz = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "max_climb_ms") == 0){
                float res = strtof(value, &endptr);
                if  (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > KX_MAX_CLIMB_MS){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %.2f.", line_counter, value, key, section, KX_MAX_CLIMB_MS);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.max_climb_ms = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "max_altitude_m") == 0){
                float res = strtof(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > KX_MAX_ALT){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %.2f.", line_counter, value, key, section, KX_MAX_ALT);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.max_altitude_m = res;
                
            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "altitude_floor_m") == 0){
                float res = strtof(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res <= (float)INV_ALT || res >= KX_MAX_ALT){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between %.2f and %.2f.", line_counter, value, key, section, (float)INV_ALT, KX_MAX_ALT);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.altitude_floor_m = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "max_operator_distance_km") == 0){
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res > KX_MAX_OP_DIST){
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and %.2f.", line_counter, value, key, section, KX_MAX_OP_DIST);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.max_operator_distance_km = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "has_sensor_pos") == 0){
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    cfg->thresholds.has_sensor_pos = true;
                } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                    cfg->thresholds.has_sensor_pos = false;
                } else {
                    log_msg(LOG_ERROR, "Error line %d: Invalid boolean value '%s' for key '%s' in section '[%s]'", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "sensor_lat") == 0){
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < MIN_SENSOR_LAT || res > MAX_SENSOR_LAT) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between %.2f and %.2f.", line_counter, value, key, section, MIN_SENSOR_LAT, MAX_SENSOR_LAT);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                if (res == 0.0 && cfg->thresholds.has_sensor_pos) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Sensor latitude cannot be zero when has_sensor_pos is true.", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.sensor_lat = res;

            } else if (strcmp(section, VALIDATOR_SECTION) == 0 && strcmp(key, "sensor_lon") == 0){
                double res = strtod(value, &endptr);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                // range validation
                if (res < MIN_SENSOR_LON || res > MAX_SENSOR_LON) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between %.2f and %.2f.", line_counter, value, key, section, MIN_SENSOR_LON, MAX_SENSOR_LON);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                if (res == 0.0 && cfg->thresholds.has_sensor_pos) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Sensor longitude cannot be zero when has_sensor_pos is true.", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }
                cfg->thresholds.sensor_lon = res;
            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_host") == 0){
                strncpy(cfg->mqtt_host, value, sizeof(cfg->mqtt_host) - 1);
                cfg->mqtt_host[sizeof(cfg->mqtt_host) - 1] = '\0';

            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_port") == 0){
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                cfg->mqtt_port = (int)res;

            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_topic") == 0){
                strncpy(cfg->mqtt_topic, value, sizeof(cfg->mqtt_topic) - 1);
                cfg->mqtt_topic[sizeof(cfg->mqtt_topic) - 1] = '\0';
            
            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_qos") == 0){
                long res = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0') return CONFIG_ERR;
                if (res < KX_MIN_MQTT_QOS || res > KX_MAX_MQTT_QOS) {
                    log_msg(LOG_ERROR, "Error line %d: Invalid value '%s' for key '%s' in section '[%s]'. Must be between 0 and 2.", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }
            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_retain") == 0){
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    cfg->mqtt_retain = true;
                } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                    cfg->mqtt_retain = false;
                } else {
                    log_msg(LOG_ERROR, "Error line %d: Invalid boolean value '%s' for key '%s' in section '[%s]'", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }

            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_enabled") == 0){
                if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    cfg->mqtt_enabled = true;
                } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                    cfg->mqtt_enabled = false;
                } else {
                    log_msg(LOG_ERROR, "Error line %d: Invalid boolean value '%s' for key '%s' in section '[%s]'", line_counter, value, key, section);
                    fclose(fp);
                    return CONFIG_ERR;
                }

            } else if (strcmp(section, MQTT_SECTION) == 0 && strcmp(key, "mqtt_creds_path") == 0){
                strncpy(cfg->mqtt_creds_path, value, sizeof(cfg->mqtt_creds_path) - 1);
                cfg->mqtt_creds_path[sizeof(cfg->mqtt_creds_path) - 1] = '\0';

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
        {"socket",                  required_argument, 0, 's'},
        {"source",                  required_argument, 0, 'f'},
        {"expiry",                  required_argument, 0, 'e'},
        {"grace",                   required_argument, 0, 'g'},
        {"max-speed",               required_argument, 0, 'm'},
        {"log-level",               required_argument, 0, 'l'},
        {"config",                  required_argument, 0, 'c'},
        {"max-operator-distance",   required_argument, 0, 'o'},
        {"teleport-threshold",      required_argument, 0, 't'},
        {"id-churn-limit",          required_argument, 0, 'k'},
        {"min-broadcast-hz",        required_argument, 0, 'v'},
        {"max-climb-ms",            required_argument, 0, 'j'},
        {"altitude-floor-m",        required_argument, 0, 'a'},
        {"max-altitude-m",          required_argument, 0, 'z'},
        {"has-sensor-pos",          required_argument, 0, 'x'},
        {"sensor-lat",              required_argument, 0, 'y'},
        {"sensor-lon",              required_argument, 0, 'w'},
        {0, 0, 0, 0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "s:f:e:g:m:l:c:o:t:k:v:j:a:z:x:y:w:", opts, NULL)) != -1){
        switch (c){
            case 's':
                strncpy(cfg->socket_path, optarg, sizeof(cfg->socket_path) - 1);
                cfg->socket_path[sizeof(cfg->socket_path) - 1] = '\0';
                break;
            case 'f':
                strncpy(cfg->source_path, optarg, sizeof(cfg->source_path) - 1);
                cfg->source_path[sizeof(cfg->source_path) - 1] = '\0';
                break;
            case 'e':
                cfg->expiry_seconds = atoi(optarg);
                break;
            case 'g':
                cfg->thresholds.grace_seconds = atoi(optarg);
                break;
            case 'm':
                cfg->thresholds.max_speed_kmh = atof(optarg);
                break;
            case 'l':
                cfg->log_level = atoi(optarg);
                break;
            case 'c':
                break;
            case 'o':
                cfg->thresholds.max_operator_distance_km = atof(optarg);
                break;
            case 't':
                cfg->thresholds.teleport_threshold = atof(optarg);
                break;
            case 'k':
                cfg->thresholds.id_churn_limit = atoi(optarg);
                break;
            case 'v':
                cfg->thresholds.min_broadcast_hz = atof(optarg);
                break;
            case 'j':
                cfg->thresholds.max_climb_ms = atof(optarg);
                break;
            case 'a':
                cfg->thresholds.altitude_floor_m = atof(optarg);
                break;
            case 'z':
                cfg->thresholds.max_altitude_m = atof(optarg);
                break;
            case 'x':
                cfg->thresholds.has_sensor_pos = (strcmp(optarg, "true") == 0 || strcmp(optarg, "1") == 0);
                break;
            case 'y':
                cfg->thresholds.sensor_lat = atof(optarg);
                break;
            case 'w':
                cfg->thresholds.sensor_lon = atof(optarg);
                break;
            default:
                return CONFIG_ERR;
        }
    }
    return APPLY_CONFIG_CLI_COMPLETE;
}
