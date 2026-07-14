#include <stdio.h>
#include "rook_track.h"
#include <string.h>
#include "capture.h"

int process_decoded_message(track_table_t *table, const decoded_message_t *msg){
	if (table == NULL || msg == NULL){
		return TRACK_ERR;
	}
	const char *uas_id = NULL;
	if (msg->type == MESSAGETYPE_BASIC_ID){
		uas_id = msg->data.basic_id.UASID;
	}

	if (!uas_id){
		if (table->num_tracks == 0){
			return TRACK_NOT_FOUND;
		}
		return track_table_update(&table->tracks[0], msg);
	}
	int search = find_track(table, uas_id);
	if (search == TRACK_NOT_FOUND){
		search = track_table_create(table, uas_id);
		if (search < 0){
			return search;
		}
	}
	return track_table_update(&table->tracks[search], msg);
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
