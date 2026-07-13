#include <stdio.h>
#include "rook_track.h"
#include <string.h>
#include "capture.h"

void track_table_init(track_table_t *table){
	if (!table){
		return;
	}
	memset(table, 0, sizeof(track_table_t));
}

int track_table_expire(track_table_t *table, time_t now, int expiry_seconds){
	int removed = 0;
	int i = 0;
	while (i < table->num_tracks){
		if (now - table->tracks[i].last_seen > expiry_seconds){
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

int track_table_create(track_table_t *table, const char *uas_id){
	if (!table || !uas_id){
		return TRACK_ERR;
	}
	if (table->num_tracks >= MAX_TRACKS){
		return TRACK_TABLE_FULL;
	}
	int track_num = table->num_tracks;
	memset(&table->tracks[track_num], 0, sizeof(track_t));
	strncpy(table->tracks[track_num].uas_id, uas_id, 20);
	table->tracks[track_num].uas_id[20] = '\0';
	table->tracks[track_num].first_seen = time(NULL);
	table->tracks[track_num].last_seen = table->tracks[track_num].first_seen;
	table->num_tracks++;
	return track_num;
}

int track_table_update(track_t *drone_track, const decoded_message_t *msg){
	if (!drone_track || !msg){
		return TRACK_ERR;
	}
	switch (msg->type){
		case MESSAGETYPE_BASIC_ID:
			drone_track->basic_id = msg->data.basic_id;
			strncpy(drone_track->uas_id, msg->data.basic_id.UASID, 20);
			drone_track->uas_id[20] = '\0';
			drone_track->has_basic_id = 1;
			drone_track->msg_count_basic_id++;
			break;
		case MESSAGETYPE_LOCATION:
			drone_track->location = msg->data.location;
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
	drone_track->last_seen = time(NULL);
	return TRACK_OK;
}

int process_decoded_message(track_table_t *table, const decoded_message_t *msg){
	if (!table || !msg){
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

