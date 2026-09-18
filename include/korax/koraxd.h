#define MQTT_TOPIC_SIZE 128
#define CONFIG_PATH "/etc/korax/korax.conf"

typedef struct {
    FILE *source;
    int source_fd;
    int hci_fd;
    server_t server;      
    track_table_t table;      
    time_t last_sweep;  
    volatile sig_atomic_t running;
    int expiry_seconds;
    magpie_thresholds_t thresholds;
    struct mosquitto *mqtt;
    char mqtt_topic[MQTT_TOPIC_SIZE];
    double sensor_lat;
    double sensor_lon;
    bool   has_sensor_pos;
    int mqtt_qos;
    bool mqtt_retain;
} koraxd_t;
