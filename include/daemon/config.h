#include <stdio.h>
#define EXPIRY_SECONDS 10          /* RID broadcasts ~1 Hz; 10s tolerates loss */
#define KX_DEFAULT_GRACE 5
#define KX_DEFAULT_MAX_SPEED 150.0
#define KX_DEFAULT_LOG_LEVEL 2

typedef struct {
    char   socket_path[108];
    char   source_path[256];
    int    expiry_seconds;
    int    grace_seconds;
    double max_speed_kmh;
    int    log_level;
} korax_config_t;

int config_defaults(korax_config_t *cfg);
int config_load_file(korax_config_t *cfg, const char *path);
