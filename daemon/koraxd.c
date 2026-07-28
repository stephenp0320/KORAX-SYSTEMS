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
#include "daemon/config.h"
#include "daemon/log.h"
#include "korax/output_mqtt.h"
#define MQTT_TOPIC_SIZE 128

typedef struct {
    FILE                 *source;      /* frame source (open file)            */
    int                   source_fd;   /* fileno(source), for poll            */
    server_t              server;      /* the KAN-45 socket feed              */
    track_table_t         table;       /* live tracks                         */
    time_t                last_sweep;  /* when expiry last ran                */
    volatile sig_atomic_t running;     /* cleared by SIGINT/SIGTERM           */
    int expiry_seconds;
    magpie_thresholds_t thresholds;
    struct mosquitto *mqtt;
    char mqtt_topic[MQTT_TOPIC_SIZE];
} koraxd_t;

static koraxd_t *g_daemon = NULL;

static void handle_stop_signal(int sig){
    (void)sig;
    if (g_daemon != NULL)
        g_daemon->running = 0;
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

static void on_track_expired(const track_t *track, void *user_data){
    if (track == NULL || user_data == NULL) return;
    koraxd_t *daemon = (koraxd_t *)user_data;
    char line[640];
    int len = snprintf(line, sizeof line, "{\"event\":\"track_expired\",\"uas_id\":\"%s\"}\n", track->uas_id);
    if (len < 0 || (size_t)len >= sizeof line) return;
    server_broadcast(&daemon->server, line, (size_t)len);
}

static void broadcast_track_update(koraxd_t *daemon, int idx){
    if (idx < 0 || idx >= daemon->table.num_tracks) return;
    char jbuf[1024], line[1280];
    if (track_to_json(&daemon->table.tracks[idx], jbuf, sizeof jbuf) < 0) return;

    int len = snprintf(line, sizeof line, "{\"event\":\"track_update\",\"track\":%s}\n", jbuf);
    if (len > 0 && (size_t)len < sizeof line){
        server_broadcast(&daemon->server, line, (size_t)len);
    }
#ifdef KORAX_WITH_MQTT
    if (daemon->mqtt){
        mqtt_publish(daemon->mqtt, NULL, daemon->mqtt_topic,
                     (int)strlen(jbuf), jbuf, 0, false);
    }
#endif
}

static void handle_source_frame(koraxd_t *daemon){
    char linebuf[512];
    if (fgets(linebuf, sizeof linebuf, daemon->source) == NULL){
        rewind(daemon->source);
        return;
    }
    char *p = linebuf;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0'){
        return;
    }
    uint8_t buf[256];
    int count = hexline_to_bytes(p, buf, sizeof buf);
    if (count <= 0) return;

    uint8_t type = (buf[0] >> 4) & 0x0F;
    if (type == MESSAGETYPE_PACKED){
        decode_message_pack(buf, count, &daemon->table, &daemon->thresholds);
        for (int i = 0; i < daemon->table.num_tracks; i++){
            broadcast_track_update(daemon, i);
        }
    } else {
        decoded_message_t msg;
        if (decode_message(buf, count, &msg) == DECODE_OK){
            int idx = process_decoded_message(&daemon->table, &msg, &daemon->thresholds, NULL);
            if (idx >= 0){
                broadcast_track_update(daemon, idx);
            }
        } else {
            log_decode_failure(time(NULL));
        }
    }
}

static void run_event_loop(koraxd_t *daemon){
    while (daemon->running){
        struct pollfd fds[2 + KX_MAX_CLIENTS];
        int n = 0;
        fds[n].fd = daemon->server.listen_fd; fds[n].events = POLLIN; n++;
        fds[n].fd = daemon->source_fd;        fds[n].events = POLLIN; n++;
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

        if (fds[0].revents & POLLIN){
            server_accept(&daemon->server, &daemon->table);
        }
        if (fds[1].revents & POLLIN){
            handle_source_frame(daemon);
        }

        for (int i = n - 1; i >= 2; i--){
            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)){
                server_drop_client(&daemon->server, (size_t)(i - 2));
            }
        }
    }
}

int main(int argc, char **argv){
    korax_config_t cfg;
    config_defaults(&cfg);
    log_set_level(cfg.log_level);

    const char *config_path = "/etc/korax/korax.conf";

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
    daemon.thresholds.grace_seconds = cfg.grace_seconds;
    daemon.thresholds.max_speed_kmh = cfg.max_speed_kmh;
    daemon.running = 1;
    daemon.last_sweep = time(NULL);
    daemon.expiry_seconds = cfg.expiry_seconds;

    track_table_init(&daemon.table);
    #ifdef KORAX_WITH_MQTT
        daemon.mqtt = mqtt_init(cfg.mqtt_enabled);
        if (daemon.mqtt){
            mqtt_connect(daemon.mqtt, cfg.mqtt_host, cfg.mqtt_port, 30);
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

    setup_signal_handling(&daemon);
    log_msg(LOG_INFO, "feed on %s, replaying %s", cfg.socket_path, cfg.source_path);
    run_event_loop(&daemon);

    #ifdef KORAX_WITH_MQTT
    if (daemon.mqtt){
        mqtt_disconnect(daemon.mqtt);
        mqtt_destroy(daemon.mqtt);
    }
    #endif
    fclose(daemon.source);
    server_close(&daemon.server);
    log_msg(LOG_INFO, "stopped");
    return 0;
}
