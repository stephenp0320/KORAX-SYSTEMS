#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include "korax/capture.h"
#include <math.h>

void track_table_init(track_table_t *table){
    if (table == NULL) return;
    memset(table, 0, sizeof(track_table_t));
}

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

static void track_table_evict_oldest(track_table_t *table){
    int oldest_track = 0;
    for (int curr_track = 1; curr_track < table->num_tracks; curr_track++){
        if (table->tracks[curr_track].last_seen < table->tracks[oldest_track].last_seen){
            oldest_track = curr_track;
        }
    }
    for (int j = oldest_track; j < table->num_tracks - 1; j++){
        table->tracks[j] = table->tracks[j + 1];
    }
    table->num_tracks--;
}

int track_table_create(track_table_t *table, time_t now, const char *uas_id){
    if (table == NULL || uas_id == NULL) return TRACK_ERR;
    if (table->num_tracks >= MAX_TRACKS){
        track_table_evict_oldest(table);
        if (table->num_tracks >= MAX_TRACKS) return TRACK_TABLE_FULL;
    }

    int track_num = table->num_tracks;
    memset(&table->tracks[track_num], 0, sizeof(track_t));
    strncpy(table->tracks[track_num].uas_id, uas_id, 20);

    table->tracks[track_num].uas_id[20] = '\0';
    table->tracks[track_num].first_seen = now;
    table->tracks[track_num].last_seen = table->tracks[track_num].first_seen;
    table->num_tracks++;
    return track_num;
}

int track_table_create_by_source_mac(track_table_t *table, time_t now, const uint8_t *source_mac){
    if (table == NULL || source_mac == NULL) return TRACK_ERR;
    if (table->num_tracks >= MAX_TRACKS){
        track_table_evict_oldest(table);
        if (table->num_tracks >= MAX_TRACKS) return TRACK_TABLE_FULL;
    }

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

double jump_detection(const track_t *table){
    double phi1 = to_radians(table->prev_location.Latitude);
    double phi2 = to_radians(table->location.Latitude);
    double delta_phi = to_radians(table->location.Latitude - table->prev_location.Latitude);
    double delta_lambda = to_radians(table->location.Longitude - table->prev_location.Longitude);
    double a = sin(delta_phi / 2.0) * sin(delta_phi / 2.0) + cos(phi1) * cos(phi2) * sin(delta_lambda / 2.0) * sin(delta_lambda / 2.0);
    double c = 2.0 * asin(sqrt(a));
    return EARTH_RADIUS_KM * c;
}

int track_table_update(track_t *drone_track, time_t now, const decoded_message_t *msg){
    if (drone_track == NULL || msg == NULL) return TRACK_ERR;

    switch (msg->type){
        case MESSAGETYPE_BASIC_ID:
            if (drone_track->has_basic_id && strncmp(drone_track->uas_id, msg->data.basic_id.UASID, 20) != 0){
                drone_track->id_change_count++;
            }
            drone_track->basic_id = msg->data.basic_id;
            strncpy(drone_track->uas_id, msg->data.basic_id.UASID, 20);
            drone_track->uas_id[20] = '\0';
            drone_track->has_basic_id = 1;
            drone_track->msg_count_basic_id++;
            break;
        case MESSAGETYPE_LOCATION:
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
    drone_track->arrival_ms[drone_track->head] = (uint64_t)now * 1000;
    drone_track->head = (drone_track->head + 1) % KX_RATE_WINDOW;
    drone_track->last_seen = now;
    return TRACK_OK;
}
