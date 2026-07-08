#include <stdio.h>
#include "track_table.h"
#include <string.h>
#include "capture.h"

int find_track(track_table_t *tracks, const char *uas_id){
	if (!tracks || !uas_id){
		return TRACK_NOT_FOUND;
	}
	int curr_track;
	for (curr_track = 0; curr_track < tracks->num_tracks; curr_track++){
		if (strcmp(tracks->tracks[curr_track].uas_id, uas_id) == 0){
			return curr_track;
		} 
	}
	return TRACK_NOT_FOUND;
}

int track_table_create(){
	return 0;
}

int track_table_update(track_t *drone_track, decoded_message_t *msg){
	if (!drone_track || !msg){
		return TRACK_ERR;
	}
		switch (msg->type){
		case MESSAGETYPE_BASIC_ID:
			drone_track->basic_id = msg->data.basic_id;
			drone_track->has_basic_id = 1;
			break;
		case MESSAGETYPE_LOCATION:
			drone_track->location = msg->data.location;
			drone_track->has_location = 1;
			break;
		case MESSAGETYPE_SYSTEM:
			drone_track->system = msg->data.system;
			drone_track->has_system = 1;
			break;
		case MESSAGETYPE_SELF_ID:
			drone_track->self_id = msg->data.self_id;
			drone_track->has_self_id = 1;
			break;
		case MESSAGETYPE_OPERATOR_ID:
			drone_track->operator_id = msg->data.operator_id;
			drone_track->has_operator_id = 1;
			break;
		case MESSAGETYPE_AUTH:
			drone_track->auth = msg->data.auth;
			drone_track->has_auth = 1;
			break;
		default:
			return TRACK_ERR;
	}
	return 0;
}

void process_ble_frame(const ble_adv_frame_t *ble_frame, track_t *drone_track){
	uint8_t rid_msg[RID_MSG_SIZE];
	decoded_message_t msg;
	if (ble_rid_extraction(ble_frame, rid_msg) != 0){
		return;
	}
	if (decode_message(rid_msg, RID_MSG_SIZE, &msg) != 0){
		return;
	}

	track_table_update(drone_track, &msg);
}
