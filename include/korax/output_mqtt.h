#ifndef KORAX_OUTPUT_MQTT_H
#define KORAX_OUTPUT_MQTT_H
#include <stdio.h>
#include <stdbool.h>
#include "daemon/config.h"
#define KX_USR "username"
#define KX_PWD "password"
#define KX_MQTT_RECONNECT_DELAY_S     2
#define KX_MQTT_RECONNECT_DELAY_MAX_S 30

struct mosquitto *mqtt_init(bool optional_mqtt);
int  mqtt_connect(struct mosquitto *mosq, const char *host, int port, int keep_alive, korax_config_t *cfg);
int  mqtt_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);
int  mqtt_disconnect(struct mosquitto *mosq);
void mqtt_destroy(struct mosquitto *mosq);
int  mqtt_loop_start(struct mosquitto *mosq);
int  mqtt_loop_stop(struct mosquitto *mosq, bool force);
int  mqtt_reconnect(struct mosquitto *mosq);
int mossquito_tls_connect(struct mosquitto *mosq, korax_config_t *cfg);

#endif
