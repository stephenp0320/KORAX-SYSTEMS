#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include "korax/capture.h"
#include "korax/magpie_validator.h"
#include <stdbool.h>

int process_decoded_message(track_table_t *table, const decoded_message_t *msg,  const magpie_thresholds_t *th){
	if (table == NULL || msg == NULL){
		return TRACK_ERR;
	}
    int idx;
	if (msg->type == MESSAGETYPE_BASIC_ID){
        const char *uas_id = msg->data.basic_id.UASID;
        idx = find_track(table, uas_id);
		if (idx == TRACK_NOT_FOUND) {
            idx = track_table_create(table, uas_id);
            if (idx < 0){
                return idx;
            }
        }
    } else {
		if (table->num_tracks == 0){
			return TRACK_NOT_FOUND;
		}
        idx = 0;
	}
    track_t *track = &table->tracks[idx];
    int rc = track_table_update(track, msg);
    if (rc != TRACK_OK){
        return rc;
    }
    track->reserved_bits_seen |= msg->reserved_bits;
    kx_verdict_t verdict = magpie_evaluate(track, msg, time(NULL), th);
    track->flags = verdict.flags;
    track->is_compliant = verdict.compliant;
    track->finding_count = verdict.finding_count;
    memcpy(track->findings, verdict.findings, sizeof track->findings);
    return idx;
}

void process_ble_frame(const ble_adv_frame_t *ble_frame, track_table_t *table){
	uint8_t rid_msg[RID_MSG_SIZE];
    uint8_t msg_counter = 0;
	decoded_message_t msg;
	if (ble_rid_extraction(ble_frame, rid_msg, &msg_counter) != 0){
		return;
	}
	if (decode_message(rid_msg, RID_MSG_SIZE, &msg) != 0){
		return;
	}
	int idx = process_decoded_message(table, &msg, &MAGPIE_DEFAULT_THRESHOLDS);
    if (idx >= 0){
        table->tracks[idx].last_rssi_dbm = ble_frame->rssi_dbm;
        table->tracks[idx].last_msg_counter = msg_counter;
    }
}

