#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include "korax/rook_track.h"
#include "korax/raven_decode.h"
#include "korax/pipeline.h"
#include "korax/server.h"
#include "korax/output_json.h"
#include "korax/scanner.h"
#include "daemon/config.h"
#include "daemon/log.h"
#include "korax/output_mqtt.h"
#include "korax/koraxd.h"
#define BUF_SIZE 1024
#define EXPORT_SUCCESS 0
#define EXPORT_ERR -1

static koraxd_t *g_daemon = NULL;

static void handle_stop_signal(int sig){
    (void)sig;
    if (g_daemon != NULL) g_daemon->running = 0;
}

static void setup_signal_handling(koraxd_t *daemon){
    g_daemon = daemon;
    struct sigaction sa;
    sa.sa_handler = handle_stop_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
}

static int hexline_to_bytes(const char *line, uint8_t *out, int max){
    int n = 0;
    const char *p = line;
    while (n < max){
        unsigned int byte;
        int consumed = 0;
        if (sscanf(p, " %2x%n", &byte, &consumed) != 1 || consumed == 0)
            break;
        out[n++] = (uint8_t)byte;
        p += consumed;
    }
    return n;
}

static int export_track_to_json(const track_t *track){
    if (track == NULL) return EXPORT_ERR;
    
    size_t fbuf_size = BUF_SIZE;
    char *fbuf = malloc(fbuf_size);
    if (fbuf == NULL) return EXPORT_ERR;

    int json_content = track_to_json(track, fbuf, fbuf_size);
    if (json_content < 0) {
        log_msg(LOG_ERROR, "Failed to convert track to JSON");
        free(fbuf);
        return EXPORT_ERR;
    }

    FILE *fp = fopen(g_daemon->track_export_path, "a");
    if (fp == NULL){
        log_msg(LOG_ERROR, "Failed to open %s for writing: %s", g_daemon->track_export_path, strerror(errno));        
        free(fbuf);
        return EXPORT_ERR;
    }
    
    size_t written = fwrite(fbuf, 1, (size_t)json_content, fp);
    if (written == (size_t)json_content){
        fputc('\n', fp);
    }
    fclose(fp);
    free(fbuf);
    return (written == (size_t)json_content) ? EXPORT_SUCCESS : EXPORT_ERR;
}

static void on_track_expired(const track_t *track, void *user_data){
    if (track == NULL || user_data == NULL) return;
    koraxd_t *daemon = (koraxd_t *)user_data;
    char line[640];
    int len = snprintf(line, sizeof line, "{\"event\":\"track_expired\",\"uas_id\":\"%s\"}\n", track->uas_id);
    if (len < 0 || (size_t)len >= sizeof line) return;
    server_broadcast(&daemon->server, line, (size_t)len);
    export_track_to_json(track);
}

static void broadcast_track_update(koraxd_t *daemon, int idx){
    if (idx < 0 || idx >= daemon->table.num_tracks) return;
    char jbuf[1024], line[1280];
    if (track_to_json(&daemon->table.tracks[idx], jbuf, sizeof jbuf) < 0) return;

    int len = snprintf(line, sizeof line, "{\"event\":\"track_update\",\"track\":%s}\n", jbuf);
    if (len > 0 && (size_t)len < sizeof line) server_broadcast(&daemon->server, line, (size_t)len);

#ifdef KORAX_WITH_MQTT
    if (daemon->mqtt) mqtt_publish(daemon->mqtt, NULL, daemon->mqtt_topic, (int)strlen(jbuf), jbuf, daemon->mqtt_qos, daemon->mqtt_retain);
#endif

}

static void handle_source_frame(koraxd_t *daemon){
    time_t now = time(NULL);
    char linebuf[512];
    if (fgets(linebuf, sizeof linebuf, daemon->source) == NULL){
        rewind(daemon->source);
        return;
    }
    char *p = linebuf;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') return;
    
    uint8_t buf[256];
    int count = hexline_to_bytes(p, buf, sizeof buf);
    if (count <= 0) return;

    uint8_t type = (buf[0] >> 4) & 0x0F;
    if (type == MESSAGETYPE_PACKED){
        decode_message_pack(buf, count, now, &daemon->table, &daemon->thresholds);
        for (int i = 0; i < daemon->table.num_tracks; i++){
            broadcast_track_update(daemon, i);
        }
    } else {
        decoded_message_t msg;
        if (decode_message(buf, count, &msg) == DECODE_OK){
            int idx = process_decoded_message(&daemon->table, &msg, now, &daemon->thresholds, NULL, NULL, &MAGPIE_DEFAULT_PROFILE);
            if (idx >= 0)broadcast_track_update(daemon, idx);
            if (idx == TRACK_TABLE_FULL) log_table_full(time(NULL));
        } else {
            log_decode_failure(time(NULL));
        }
    }
}

static void handle_hci_event(koraxd_t *daemon, time_t now){
    int idx = -1;
    if (hci_scan_read(daemon->hci_fd, &daemon->table, now, &daemon->thresholds, &MAGPIE_DEFAULT_PROFILE, &idx) != SCAN_OK){
        log_msg(LOG_ERROR, "HCI socket read failed, disabling live capture");
        hci_scan_close(daemon->hci_fd);
        daemon->hci_fd = -1;
        return;
    }

    if (idx >= 0) broadcast_track_update(daemon, idx);
    if (idx == TRACK_TABLE_FULL) log_table_full(now);
}

static void run_event_loop(koraxd_t *daemon){
    while (daemon->running){
        struct pollfd fds[3 + KX_MAX_CLIENTS];
        int n = 0;
        fds[n].fd = daemon->server.listen_fd; fds[n].events = POLLIN; n++;
        fds[n].fd = daemon->source_fd;        fds[n].events = POLLIN; n++;
        fds[n].fd = daemon->hci_fd;           fds[n].events = POLLIN; n++;
        for (size_t i = 0; i < daemon->server.client_count; i++){
            fds[n].fd = daemon->server.client_fds[i]; fds[n].events = POLLIN; n++;
        }

        int ready = poll(fds, n, 1000);
        if (ready == -1){
            if (errno == EINTR) continue;
            break;
        }

        time_t now = time(NULL);
        if (now - daemon->last_sweep >= daemon->expiry_seconds){
            track_table_expire(&daemon->table, now, daemon->expiry_seconds, on_track_expired, daemon);
            daemon->last_sweep = now;
        }

        if (ready == 0) continue;
        if (fds[0].revents & POLLIN) server_accept(&daemon->server, &daemon->table);
        if (fds[1].revents & POLLIN) handle_source_frame(daemon);
        if (fds[2].revents & POLLIN) handle_hci_event(daemon, now);

        for (int i = n - 1; i >= 3; i--){
            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)){
                server_drop_client(&daemon->server, (size_t)(i - 3));
            }
        }
    }
}

int main(int argc, char **argv){
    korax_config_t cfg;
    config_defaults(&cfg);
    log_set_level(cfg.log_level);

    const char *config_path = CONFIG_PATH;
    const char *env_path = getenv("KORAX_CONFIG_PATH");
    if (config_path == NULL && env_path != NULL) config_path = env_path;
    
    for (int i = 1; i < argc - 1; i++){
        if (strcmp(argv[i], "--config") == 0){
            config_path = argv[i + 1];
            break;
        }
    }
    
    if (config_load_file(&cfg, config_path) != 0){
        log_msg(LOG_ERROR, "failed to load config %s", config_path);
        return 1;
    }

    if (config_apply_cli(&cfg, argc, argv) != 0){
        log_msg(LOG_ERROR, "invalid command-line arguments");
        return 1;
    }

    koraxd_t daemon;
    memset(&daemon, 0, sizeof daemon);
    daemon.thresholds = cfg.thresholds;
    daemon.running = 1;
    daemon.last_sweep = time(NULL);
    daemon.expiry_seconds = cfg.expiry_seconds;
    snprintf(daemon.track_export_path, sizeof daemon.track_export_path, "%s", cfg.track_export_path);
    track_table_init(&daemon.table);
    #ifdef KORAX_WITH_MQTT
        daemon.mqtt = mqtt_init(cfg.mqtt_enabled);
        if (daemon.mqtt){
            mqtt_reconnect(daemon.mqtt);
            if (cfg.mqtt_tls_ca_path[0] != '\0') mossquito_tls_connect(daemon.mqtt, &cfg);
            if (mqtt_connect(daemon.mqtt, cfg.mqtt_host, cfg.mqtt_port, 60, &cfg) != 0){
                log_msg(LOG_ERROR, "mqtt_connect failed to %s:%d", cfg.mqtt_host, cfg.mqtt_port);
                mqtt_destroy(daemon.mqtt);
                daemon.mqtt = NULL;
            } else {
                log_msg(LOG_INFO, "mqtt connected to %s:%d", cfg.mqtt_host, cfg.mqtt_port);
                if (mqtt_loop_start(daemon.mqtt) != 0){
                    mqtt_disconnect(daemon.mqtt);
                    mqtt_destroy(daemon.mqtt);
                    daemon.mqtt = NULL;
                }
            }
            snprintf(daemon.mqtt_topic, sizeof daemon.mqtt_topic, "%s", cfg.mqtt_topic);
        }
    #endif
    if (server_init(&daemon.server, cfg.socket_path) != 0){
        log_msg(LOG_ERROR, "server_init failed on %s", cfg.socket_path);
        return 1;
    }

    daemon.source = fopen(cfg.source_path, "r");
    if (daemon.source == NULL){
        log_msg(LOG_ERROR, "cannot open source %s: %s", cfg.source_path, strerror(errno));
        server_close(&daemon.server);
        return 1;
    }
    daemon.source_fd = fileno(daemon.source);

    daemon.hci_fd = hci_scan_open();
    if (daemon.hci_fd >= 0){
        log_msg(LOG_INFO, "live BLE capture enabled (HCI socket fd=%d)", daemon.hci_fd);
    } else {
        log_msg(LOG_INFO, "live BLE capture unavailable, replay-only");
    }

    setup_signal_handling(&daemon);
    log_msg(LOG_INFO, "feed on %s, replaying %s", cfg.socket_path, cfg.source_path);
    run_event_loop(&daemon);

    #ifdef KORAX_WITH_MQTT
    if (daemon.mqtt){
        mqtt_loop_stop(daemon.mqtt, true);
        mqtt_disconnect(daemon.mqtt);
        mqtt_destroy(daemon.mqtt);
    }
    #endif

    if (daemon.hci_fd >= 0) hci_scan_close(daemon.hci_fd);
    fclose(daemon.source);
    server_close(&daemon.server);
    log_msg(LOG_INFO, "stopped");
    return 0;
}
