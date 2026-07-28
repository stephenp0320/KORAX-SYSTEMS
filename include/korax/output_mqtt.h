#ifndef KORAX_OUTPUT_MQTT_H
#define KORAX_OUTPUT_MQTT_H
#include <stdio.h>
#include <stdbool.h>

struct mosquitto *mqtt_init(bool optional_mqtt);
int  mqtt_connect(struct mosquitto *mosq, const char *host, int port, int keep_alive);
int  mqtt_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);
int  mqtt_disconnect(struct mosquitto *mosq);
void mqtt_destroy(struct mosquitto *mosq);
#endif
