/* Optional MQTT output adapter — only built/linked when KORAX_WITH_MQTT is
 * enabled (needs libmosquitto). Off by default; not part of the core
 * pipeline, which stays I/O-free. Mirrors the NDJSON feed from server.c
 * onto an MQTT topic for consumers that prefer pub/sub over the socket. */
#ifndef KORAX_OUTPUT_MQTT_H
#define KORAX_OUTPUT_MQTT_H
#include <stdio.h>
#include <stdbool.h>
#include "daemon/config.h"
#define KX_USR "username"
#define KX_PWD "password"

struct mosquitto *mqtt_init(bool optional_mqtt);

int  mqtt_connect(struct mosquitto *mosq, const char *host, int port, int keep_alive, korax_config_t *cfg);

int  mqtt_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

int  mqtt_disconnect(struct mosquitto *mosq);

void mqtt_destroy(struct mosquitto *mosq);

int mossquito_tls_connect(struct mosquitto *mosq, korax_config_t *cfg);

#endif
