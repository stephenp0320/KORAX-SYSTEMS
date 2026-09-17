/*
 * Compiled-in defaults and the config struct for koraxd. See config.c
 * for the load order (defaults -> INI file -> CLI) and the rule that
 * every new numeric knob needs a default here plus a parse branch there.
 */
#ifndef KORAX_DAEMON_CONFIG_H
#define KORAX_DAEMON_CONFIG_H
#include <stdio.h>
#include <stdbool.h>
#include "korax/magpie_validator.h"
#define EXPIRY_SECONDS       10
#define KX_DEFAULT_GRACE     5
#define KX_DEFAULT_MAX_SPEED 150.0
#define KX_DEFAULT_LOG_LEVEL 2
#define KX_DEFAULT_MIN_BROADCAST_HZ 0.5
#define KX_DEFAULT_MAX_CLIMB 20.0
#define KX_DEFAULT_ALT_FLOOR -500.0
#define KX_DEFAULT_MAX_ALT MAX_ALT
#define KX_DEFAULT_MAX_OP_DIST 5.0
#define KX_DEFAULT_MQTT_ENABLED false
#define KX_DEFAULT_MQTT_HOST    "localhost"
#define KX_DEFAULT_MQTT_PORT    1883
#define KX_DEFAULT_MQTT_TOPIC   "korax/tracks"
#define KX_DEFAULT_SENSOR_LAT 0.0
#define KX_DEFAULT_SENSOR_LON 0.0
#define KX_DEFAULT_HAS_SENSOR_LOCATION false
#define KX_DEFAULT_ID_CHURN_LIMIT 2
#define KX_DEFAULT_TELEPORT_THRESHOLD 5000.0
#define KX_MAX_EXPIRY_SECONDS 300
#define KX_MAX_GRACE_SECONDS 60

/* Mirrors [output]/[capture]/[tracks]/[validator]/[log]/[mqtt] INI sections. */
typedef struct {
    char   socket_path[108];
    char   source_path[256];
    int    expiry_seconds;
    int    log_level;
    bool   mqtt_enabled;
    char   mqtt_host[256];
    int    mqtt_port;
    char   mqtt_topic[128];
    char   mqtt_tls_ca_path[256];
    char   mqtt_tls_cert_path[256]; 
    char   mqtt_tls_key_path[256];
    magpie_thresholds_t thresholds;
} korax_config_t;

int config_defaults(korax_config_t *cfg);
int config_load_file(korax_config_t *cfg, const char *path);
int config_apply_cli(korax_config_t *cfg, int argc, char **argv);
#endif
