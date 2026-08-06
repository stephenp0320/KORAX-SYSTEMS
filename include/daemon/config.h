/*
 * Compiled-in defaults and the config struct for koraxd. See config.c
 * for the load order (defaults -> INI file -> CLI) and the rule that
 * every new numeric knob needs a default here plus a parse branch there.
 */
#include <stdio.h>
#include <stdbool.h>

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

/* Mirrors [output]/[capture]/[tracks]/[validator]/[log]/[mqtt] INI sections. */
typedef struct {
    char   socket_path[108];
    char   source_path[256];
    int    expiry_seconds;
    int    grace_seconds;
    double max_speed_kmh;
    double min_broadcast_hz;
    float max_climb_ms;
    float altitude_floor_m;
    float max_altitude_m;
    double max_operator_distance_km;
    int    log_level;
    bool   mqtt_enabled;
    char   mqtt_host[256];
    int    mqtt_port;
    char   mqtt_topic[128];
    double sensor_lat;
    double sensor_lon;
    bool   has_sensor_pos;
} korax_config_t;

int config_defaults(korax_config_t *cfg);
int config_load_file(korax_config_t *cfg, const char *path);
int config_apply_cli(korax_config_t *cfg, int argc, char **argv);
