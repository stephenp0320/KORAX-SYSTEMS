#include <stdio.h>

typedef struct {
    char   socket_path[108];
    char   source_path[256];
    int    expiry_seconds;
    int    grace_seconds;
    double max_speed_kmh;
    int    log_level;
} korax_config_t;
