#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include "daemon/log.h"
#include <errno.h>
#include "korax/output_mqtt.h"
#include "daemon/config.h"
#include <stdlib.h>
#define KX_CREDS_NOT_FOUND 1

int read_creds(char *file, korax_config_t *cfg) {
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        if (errno == ENOENT) return KX_CREDS_NOT_FOUND;
        log_msg(LOG_ERROR, "Failed to open credentials file: %s", strerror(errno));
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof line, fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';

        char *eq = strchr(line, '=');
        if (eq == NULL) continue;
        
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        
        if (strcmp(key, KX_USR) == 0) {
            strncpy(cfg->mqtt_username, val, sizeof cfg->mqtt_username - 1);
            cfg->mqtt_username[sizeof cfg->mqtt_username - 1] = '\0';
        } else if (strcmp(key, KX_PWD) == 0) {
            strncpy(cfg->mqtt_password, val, sizeof cfg->mqtt_password - 1);
            cfg->mqtt_password[sizeof cfg->mqtt_password - 1] = '\0';
        }
    }
    fclose(fp);
    return 0;
}

struct mosquitto *mqtt_init(bool optional_mqtt) {
    if (!optional_mqtt) return NULL;
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        log_msg(LOG_ERROR, "mosquitto_new failed: %s", strerror(errno));
        mosquitto_lib_cleanup();
    }
    return mosq;
}

int mossquito_tls_connect(struct mosquitto *mosq, korax_config_t *cfg) {
    if (mosq == NULL || cfg == NULL) return MOSQ_ERR_INVAL;

    int tls_connect = mosquitto_tls_set(mosq, cfg->mqtt_tls_ca_path, NULL, cfg->mqtt_tls_cert_path,
                                        cfg->mqtt_tls_key_path, NULL);
    if (tls_connect != MOSQ_ERR_SUCCESS) {
        log_msg(LOG_ERROR, "mosquitto_tls_set failed: %s", strerror(errno));
        return tls_connect;
    }
    return MOSQ_ERR_SUCCESS;
}

int mqtt_connect(struct mosquitto *mosq, const char *host, int port, int keep_alive,
                 korax_config_t *cfg) {
    if (mosq == NULL || host == NULL || port < 0 || keep_alive < 5 || cfg == NULL) return MOSQ_ERR_INVAL;

    int read_creds_result = read_creds(cfg->mqtt_creds_path, cfg);
    if (read_creds_result == KX_CREDS_NOT_FOUND) {
        log_msg(LOG_INFO, "No MQTT credentials file found; connecting without authentication");
    } else if (read_creds_result != 0) {
        log_msg(LOG_ERROR, "Failed to read credentials: %s", strerror(errno));
        return read_creds_result;
    } else if (cfg->mqtt_username[0] != '\0') {
        int username_pw = mosquitto_username_pw_set(mosq, cfg->mqtt_username, cfg->mqtt_password);
        if (username_pw != MOSQ_ERR_SUCCESS) {
            log_msg(LOG_ERROR, "mosquitto_username_pw_set failed: %s", strerror(errno));
            return MOSQ_ERR_INVAL;
        }
    }

    int broker_connection = mosquitto_connect(mosq, host, port, keep_alive);
    if (broker_connection != MOSQ_ERR_SUCCESS) {
        log_msg(LOG_ERROR, "mqtt connect failed: %s", strerror(errno));
        return broker_connection;
    }
    return MOSQ_ERR_SUCCESS;
}

int mqtt_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen,
                 const void *payload, int qos, bool retain) {
    if (mosq == NULL || topic == NULL || payloadlen < 0 || (payloadlen > 0 && payload == NULL) ||
        qos < 0 || qos > 2)
        return MOSQ_ERR_INVAL;

    int publish_message = mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos, retain);
    switch (publish_message) {
    case MOSQ_ERR_INVAL:
        log_msg(LOG_ERROR, "input parameters were invalid: %s", strerror(errno));
        return MOSQ_ERR_INVAL;
    case MOSQ_ERR_NOMEM:
        log_msg(LOG_ERROR, "out of memory condition occurred: %s", strerror(errno));
        return MOSQ_ERR_NOMEM;
    case MOSQ_ERR_NO_CONN:
        log_msg(LOG_ERROR, "client isn’t connected to a broker: %s", strerror(errno));
        return MOSQ_ERR_NO_CONN;
    case MOSQ_ERR_PROTOCOL:
        log_msg(LOG_ERROR, "protocol error communicating with the broker: %s", strerror(errno));
        return MOSQ_ERR_PROTOCOL;
    case MOSQ_ERR_PAYLOAD_SIZE:
        log_msg(LOG_ERROR, "payload length is too large: %s", strerror(errno));
        return MOSQ_ERR_PAYLOAD_SIZE;
    case MOSQ_ERR_MALFORMED_UTF8:
        log_msg(LOG_ERROR, "topic is not valid UTF-8: %s", strerror(errno));
        return MOSQ_ERR_MALFORMED_UTF8;
    case MOSQ_ERR_QOS_NOT_SUPPORTED:
        log_msg(LOG_ERROR, "the QoS is greater than that supported by the broker: %s",
                strerror(errno));
        return MOSQ_ERR_QOS_NOT_SUPPORTED;
    case MOSQ_ERR_OVERSIZE_PACKET:
        log_msg(LOG_ERROR, "the resulting packet would be larger than supported by the broker: %s",
                strerror(errno));
        return MOSQ_ERR_OVERSIZE_PACKET;
    default:
        return MOSQ_ERR_SUCCESS;
    }
}

int mqtt_disconnect(struct mosquitto *mosq) {
    if (mosq == NULL) return MOSQ_ERR_INVAL;

    int disconnect = mosquitto_disconnect(mosq);
    switch (disconnect) {
    case MOSQ_ERR_INVAL:
        log_msg(LOG_ERROR, "he input parameters were invalid: %s", strerror(errno));
        return MOSQ_ERR_INVAL;
    case MOSQ_ERR_NO_CONN:
        log_msg(LOG_ERROR, "the client isn’t connected to a broker: %s", strerror(errno));
        return MOSQ_ERR_NO_CONN;
    default:
        return MOSQ_ERR_SUCCESS;
    }
}

void mqtt_destroy(struct mosquitto *mosq) {
    if (mosq == NULL) return;
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

int mqtt_loop_start(struct mosquitto *mosq) {
    if (mosq == NULL) return MOSQ_ERR_INVAL;
    int result = mosquitto_loop_start(mosq);
    if (result != MOSQ_ERR_SUCCESS) log_msg(LOG_ERROR, "mosquitto_loop_start failed: %s", mosquitto_strerror(result));
    
    return result;
}

int mqtt_loop_stop(struct mosquitto *mosq, bool force) {
    if (mosq == NULL) return MOSQ_ERR_INVAL;
    return mosquitto_loop_stop(mosq, force);
}
