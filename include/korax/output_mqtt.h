/* Optional MQTT output adapter — only built/linked when KORAX_WITH_MQTT is
 * enabled (needs libmosquitto). Off by default; not part of the core
 * pipeline, which stays I/O-free. Mirrors the NDJSON feed from server.c
 * onto an MQTT topic for consumers that prefer pub/sub over the socket. */
#ifndef KORAX_OUTPUT_MQTT_H
#define KORAX_OUTPUT_MQTT_H
#include <stdio.h>
#include <stdbool.h>

/* Returns NULL if optional_mqtt is false (MQTT disabled by config) or on
 * mosquitto init failure -- callers must treat NULL as "no MQTT" and skip
 * publish/disconnect/destroy accordingly. */
struct mosquitto *mqtt_init(bool optional_mqtt);
/* Returns MOSQ_ERR_SUCCESS or a mosquitto MOSQ_ERR_* code; MOSQ_ERR_INVAL
 * on bad args (mosq/host NULL, port < 0, keep_alive < 5). */
int  mqtt_connect(struct mosquitto *mosq, const char *host, int port, int keep_alive);
/* Thin wrapper over mosquitto_publish; returns the underlying MOSQ_ERR_*
 * code unchanged so callers can distinguish failure modes (see .c). */
int  mqtt_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);
int  mqtt_disconnect(struct mosquitto *mosq);
/* No-op if mosq is NULL; always pairs with mqtt_init's lib_init. */
void mqtt_destroy(struct mosquitto *mosq);
#endif
