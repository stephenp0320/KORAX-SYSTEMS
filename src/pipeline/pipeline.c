#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include "korax/capture.h"
#include "korax/magpie_validator.h"
#include <stdbool.h>
int process_decoded_message(track_table_t *table, const decoded_message_t *msg){
	if (table == NULL || msg == NULL){
		return TRACK_ERR;
	}
    int idx;
	if (msg->type == MESSAGETYPE_BASIC_ID){
        const char *uas_id = msg->data.basic_id.UASID;
        idx = find_track(table, uas_id);
		if (idx == TRACK_NOT_FOUND) {
            idx = track_table_create(table, uas_id);
            if (idx < 0)
                return idx;
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
    track->flags = magpie_validate(track, msg, time(NULL));
    track->is_compliant = (track->flags & KX_RULE_ERROR_MASK) == 0;
    return idx;
}

void process_ble_frame(const ble_adv_frame_t *ble_frame, track_table_t *table){
	uint8_t rid_msg[RID_MSG_SIZE];
	decoded_message_t msg;
	if (ble_rid_extraction(ble_frame, rid_msg) != 0){
		return;
	}
	if (decode_message(rid_msg, RID_MSG_SIZE, &msg) != 0){
		return;
	}
	process_decoded_message(table, &msg);
}

