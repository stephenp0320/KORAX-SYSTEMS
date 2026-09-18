/*
 * Public API for Rook, the track table (ROOK stage of the pipeline). Defines
 * track_t (fixed-size, one per observed drone) and track_table_t, plus the
 * create/find/update/expire functions src/rook/table_track.c implements.
 */
#ifndef TRACK_TABLE_H
#define TRACK_TABLE_H
#include <time.h>
#include "korax/raven_decode.h"
#include <stdbool.h>
#include "korax/magpie_types.h"
#define MAX_TRACKS 100
#define TRACK_OK 0
#define TRACK_ERR -1
#define TRACK_NOT_FOUND -2
#define TRACK_TABLE_FULL -3
#define PI 3.14159265358979323846
#define EARTH_RADIUS_KM 6371.0
#define MAX_DIST_KM   0.10
#define MIN_LOCATION_MESSAGES_FOR_JUMP_CHECK 2

struct magpie_thresholds;
typedef struct {
    char uas_id[21];
    droneID_basicID_data basic_id;
    DRONE_ID_Location_data location;
    DRONE_ID_Location_data prev_location;  // previous fix, kept for jump/timestamp-rewind checks
    time_t prev_location_seen;
    time_t last_location_seen;
    sys_data system;
    OperatorID_data operator_id;
    selfID_data self_id;
    authentication_data auth;
    int has_basic_id;
    uint8_t source_mac[6];  // live-capture key (see find_track_by_mac); unused for file replay
    int has_auth;
    int has_location;
    int has_system;
    int has_operator_id;
    int has_self_id;
    int msg_count_basic_id;
    int msg_count_location;
    int msg_count_system;
    int msg_count_operator_id;
    int msg_count_auth;
    int msg_count_self_id;
    time_t first_seen;
    time_t last_seen;
    uint32_t flags;
    bool is_compliant;
    int reserved_bits_seen;
    uint8_t finding_count;
    kx_finding_t findings[KX_MAX_FINDINGS];
    uint64_t arrival_ms[KX_RATE_WINDOW];  // ring buffer of recent arrival times, for broadcast-rate checks
    uint8_t head;                          // next write index into arrival_ms, wraps mod KX_RATE_WINDOW
    uint8_t last_msg_counter;
    bool have_counter;
    int8_t last_rssi_dbm;
    uint8_t id_change_count;
} track_t;

typedef struct {
    track_t tracks[MAX_TRACKS];
    int num_tracks;
} track_table_t;

typedef void (*track_expired_cb)(const track_t *track, void *user_data);

void track_table_init(track_table_t *table);
int find_track(track_table_t *table, const char *uas_id);
int track_table_create(track_table_t *table, time_t now,  const char *uas_id);
int track_table_update(track_t *track, time_t now, const decoded_message_t *msg);
int decode_message_pack(uint8_t *packet, int len, time_t now, track_table_t *table, const struct magpie_thresholds *th);
int track_table_expire(track_table_t *table, time_t now, int expiry_seconds, track_expired_cb on_expired, void *user_data);
int find_track_by_mac(track_table_t *table, const uint8_t mac[6]);
int track_table_create_by_source_mac(track_table_t *table, time_t now, const uint8_t *source_mac);
double to_radians(double degrees);
double jump_detection(const track_t *table);
#endif
