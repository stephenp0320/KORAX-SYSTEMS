/*
 * Rook — the track table (ROOK stage: PIPELINE -> ROOK -> MAGPIE). Owns the
 * fixed-size track_t array: create/find/update/expire and the haversine-based
 * jump check. No heap allocation, no I/O — buffers are caller-provided.
 */
#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include "korax/capture.h"
#include <math.h>
#define PI 3.14159265358979323846
#define EARTH_RADIUS_KM 6371.0
#define MAX_DIST_KM   0.10
#define MIN_LOCATION_MESSAGES_FOR_JUMP_CHECK 2

void track_table_init(track_table_t *table){
    if (table == NULL) return;
    memset(table, 0, sizeof(track_table_t));
}

// Removes tracks idle longer than expiry_seconds, compacting the array in place.
// on_expired fires before the slot is overwritten, so it still sees a valid
// track_t; do not retain that pointer past the callback. i is only advanced
// on a non-removal — after a compaction the next track has shifted into slot i,
// so re-checking the same index is deliberate, not a bug.
int track_table_expire(track_table_t *table, time_t now, int expiry_seconds, track_expired_cb on_expired, void *user_data){
    int removed = 0;
    int i = 0;
    while (i < table->num_tracks){
        if (now - table->tracks[i].last_seen > expiry_seconds){
            if (on_expired){
                on_expired(&table->tracks[i], user_data);
            }
            for (int j = i; j < table->num_tracks - 1; j++){
                table->tracks[j] = table->tracks[j + 1];
            }
            table->num_tracks--;
            removed++;
        } else {
            i++;
        }
    }
    return removed;
}

// MAC-keyed lookup, paired with track_table_create_by_source_mac. Used only for
// live capture (process_decoded_message sees a non-NULL ble_adv_frame_t), because
// a single physical drone can broadcast several message types from one MAC before
// a Basic ID ever arrives to key by UAS-ID. File replay (frame == NULL) has no
// MAC and must use find_track/track_table_create instead — do not mix the two
// keying schemes for the same source.
// Returns a track index (>= 0) or TRACK_NOT_FOUND (negative) — callers must
// test `< 0`, never `!= 0`: index 0 is a valid track.
int find_track_by_mac(track_table_t *table, const uint8_t mac[6]){
    int i = 0;
    int curr_track = 0;
    while (i < table->num_tracks){
        if (memcmp(table->tracks[i].source_mac, mac, 6) == 0){
            curr_track = i;
            return curr_track;
        } else {
            i++;
        }
    }
    return TRACK_NOT_FOUND;
}

// UAS-ID-keyed lookup, paired with track_table_create. This is the fallback path
// for file replay (no source MAC available) — see find_track_by_mac for why the
// two keying schemes are kept separate. Same return convention: negative means
// not found/error, test `< 0` (index 0 is a valid track).
int find_track(track_table_t *tracks, const char *uas_id){
    if (tracks == NULL || uas_id == NULL) return TRACK_NOT_FOUND;

    int curr_track;
    for (curr_track = 0; curr_track < tracks->num_tracks; curr_track++){
        if (strcmp(tracks->tracks[curr_track].uas_id, uas_id) == 0){
            return curr_track;
        }
    }
    return TRACK_NOT_FOUND;
}

// Appends a new track keyed by UAS-ID (the file-replay path — see find_track).
// Returns the new track's index (>= 0), or a negative TRACK_ERR/TRACK_TABLE_FULL
// on failure; callers must check `< 0`, not `!= 0`, since index 0 is valid.
int track_table_create(track_table_t *table, time_t now, const char *uas_id){
    if (table == NULL || uas_id == NULL) return TRACK_ERR;
    if (table->num_tracks >= MAX_TRACKS) return TRACK_TABLE_FULL;

    int track_num = table->num_tracks;
    memset(&table->tracks[track_num], 0, sizeof(track_t));
    strncpy(table->tracks[track_num].uas_id, uas_id, 20);

    table->tracks[track_num].uas_id[20] = '\0';
    table->tracks[track_num].first_seen = now;
    table->tracks[track_num].last_seen = table->tracks[track_num].first_seen;
    table->num_tracks++;

    return track_num;
}

// Appends a new track keyed by source MAC (the live-capture path — see
// find_track_by_mac for why). Same index/error convention as track_table_create:
// negative is failure, test `< 0` — 0 is a valid track index.
int track_table_create_by_source_mac(track_table_t *table, time_t now, const uint8_t *source_mac){
    if (table == NULL || source_mac == NULL) return TRACK_ERR;
    if (table->num_tracks >= MAX_TRACKS) return TRACK_TABLE_FULL;

    int track_num = table->num_tracks;
    memset(&table->tracks[track_num], 0, sizeof(track_t));
    memcpy(table->tracks[track_num].source_mac, source_mac, sizeof(table->tracks[track_num].source_mac));

    table->tracks[track_num].first_seen = now;
    table->tracks[track_num].last_seen = table->tracks[track_num].first_seen;
    table->num_tracks++;

    return track_num;
}

double to_radians(double degrees){
    return degrees * (PI / 180.0);
}

// Haversine great-circle distance (km) between the track's previous and current
// location fixes. Must return double, not int: an int return truncates any
// sub-km jump to 0, silently defeating the plausibility check that calls this.
double jump_detection(const track_t *table){
    double phi1 = to_radians(table->prev_location.Latitude);
    double phi2 = to_radians(table->location.Latitude);
    double delta_phi = to_radians(table->location.Latitude - table->prev_location.Latitude);
    double delta_lambda = to_radians(table->location.Longitude - table->prev_location.Longitude);
    double a = sin(delta_phi / 2.0) * sin(delta_phi / 2.0) + cos(phi1) * cos(phi2) * sin(delta_lambda / 2.0) * sin(delta_lambda / 2.0);
    double c = 2.0 * asin(sqrt(a));
    return EARTH_RADIUS_KM * c;
}

// Folds one decoded message into an existing track (create/lookup happens
// upstream in the pipeline). Pure field update — no I/O, no allocation.
int track_table_update(track_t *drone_track, time_t now, const decoded_message_t *msg){
    if (drone_track == NULL || msg == NULL) return TRACK_ERR;

    switch (msg->type){
        case MESSAGETYPE_BASIC_ID:
            drone_track->basic_id = msg->data.basic_id;
            strncpy(drone_track->uas_id, msg->data.basic_id.UASID, 20);
            drone_track->uas_id[20] = '\0';
            drone_track->has_basic_id = 1;
            drone_track->msg_count_basic_id++;
            break;
        case MESSAGETYPE_LOCATION:
            // Save the outgoing fix before overwriting it — jump_detection and
            // the timestamp-rewind check both need the previous/current pair.
            drone_track->prev_location = drone_track->location;
            drone_track->prev_location_seen = drone_track->last_location_seen;
            drone_track->location = msg->data.location;
            drone_track->last_location_seen = now;
            drone_track->has_location = 1;
            drone_track->msg_count_location++;
            break;
        case MESSAGETYPE_SYSTEM:
            drone_track->system = msg->data.system;
            drone_track->has_system = 1;
            drone_track->msg_count_system++;
            break;
        case MESSAGETYPE_SELF_ID:
            drone_track->self_id = msg->data.self_id;
            drone_track->has_self_id = 1;
            drone_track->msg_count_self_id++;
            break;
        case MESSAGETYPE_OPERATOR_ID:
            drone_track->operator_id = msg->data.operator_id;
            drone_track->has_operator_id = 1;
            drone_track->msg_count_operator_id++;
            break;
        case MESSAGETYPE_AUTH:
            drone_track->auth = msg->data.auth;
            drone_track->has_auth = 1;
            drone_track->msg_count_auth++;
            break;
        default:
            return TRACK_ERR;
    }
    // Ring buffer of recent arrival times (any message type) for Magpie's
    // broadcast-rate check; head wraps modulo KX_RATE_WINDOW.
    drone_track->arrival_ms[drone_track->head] = (uint64_t)now * 1000;
    drone_track->head = (drone_track->head + 1) % KX_RATE_WINDOW;
    drone_track->last_seen = now;
    return TRACK_OK;
}
