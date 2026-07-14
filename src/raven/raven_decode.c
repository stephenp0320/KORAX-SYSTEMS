#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raven_decode.h"
#include "rook_track.h"
#include "pipeline.h"
#define DJIMAXSIZE 20
//#define DJI 4
#define WP_ID_MAX_LEN 20
#define ODID_PACK_MAX_MESSAGES 9

int decode_message_pack(uint8_t *packet, int packet_len, track_table_t *track_table){
    if (packet == NULL || track_table == NULL){
        return DECODE_ERR_NULL;
    }
    
    if (packet_len < 3){
        return DECODE_ERR_TOO_SHORT;
    }
    uint8_t msg_type = (packet[0] >> 4) & 0x0F;
    if (msg_type != MESSAGETYPE_PACKED){
        return DECODE_ERR_BAD_TYPE;
    }
    uint8_t single_msg_size =packet[1];
    uint8_t msg_count = packet[2]; 
    if (single_msg_size != RID_MSG_SIZE){
        return DECODE_ERR_RANGE;
    }
    if (msg_count == 0 || msg_count > ODID_PACK_MAX_MESSAGES) {
        return DECODE_ERR_RANGE;
    }

    int expected_len = 3 + (msg_count * RID_MESSAGE_SIZE);
    if (packet_len < expected_len){
        return DECODE_ERR_TOO_SHORT;
    }
   
    for (int i = 0; i < msg_count; i++){
        const uint8_t *msg_start = packet + 3 + (i * RID_MESSAGE_SIZE);
        decoded_message_t msg;

        int decode_msg_result = decode_message(msg_start, RID_MESSAGE_SIZE, &msg);
        if (decode_msg_result != 0){
            return decode_msg_result;
        }

        int processed_decoded_msg = process_decoded_message(track_table, &msg);
        if (processed_decoded_msg != 0) {
            return processed_decoded_msg;
        }
    }
    return 0;
}

int decode_message(const uint8_t *packet, int packet_len, decoded_message_t *decoded_output){
    if (packet == NULL || decoded_output == NULL){
        return DECODE_ERR_NULL;
    }
    if (packet_len < RID_MESSAGE_SIZE){
        return DECODE_ERR_TOO_SHORT;
    }
    uint8_t msg_type = (packet[0] >> 4) & 0x0F;
    switch (msg_type){
        case MESSAGETYPE_BASIC_ID:
            decoded_output->type = MESSAGETYPE_BASIC_ID;
            return decode_basic_id(packet, packet_len, &decoded_output->data.basic_id);
        case MESSAGETYPE_LOCATION:
            decoded_output->type = MESSAGETYPE_LOCATION;
            return decode_location_vector(packet, packet_len, &decoded_output->data.location);
        case MESSAGETYPE_AUTH:
            decoded_output->type = MESSAGETYPE_AUTH;
            return decode_auth(packet, packet_len, &decoded_output->data.auth);
        case MESSAGETYPE_SELF_ID:
            decoded_output->type = MESSAGETYPE_SELF_ID;
            return decode_selfID(packet, packet_len, &decoded_output->data.self_id);
        case MESSAGETYPE_SYSTEM:
            decoded_output->type = MESSAGETYPE_SYSTEM;
            return decode_system_msg(packet, packet_len, &decoded_output->data.system);
        case MESSAGETYPE_OPERATOR_ID:
            decoded_output->type = MESSAGETYPE_OPERATOR_ID;
            return decode_operator_id(packet, packet_len, &decoded_output->data.operator_id);
        case MESSAGETYPE_PACKED:
            decoded_output->type = MESSAGETYPE_INVALID;
            return DECODE_ERR_BAD_TYPE;
        default:
            decoded_output->type = MESSAGETYPE_INVALID;
            return DECODE_ERR_BAD_TYPE;
    }
}

int decode_basic_id(const uint8_t *packet, int packet_len, droneID_basicID_data *basicID){
	if (packet == NULL || basicID == NULL){
		return DECODE_ERR_NULL;
	}
	if (packet_len < DRONE_ID_PACK_SIZE){
		return DECODE_ERR_TOO_SHORT;
	}
	uint8_t msg_type = (packet[0] >> 4) & 0x0F;
	if (msg_type != MESSAGETYPE_BASIC_ID){
		return DECODE_ERR_BAD_TYPE;
	}
	basicID->UAType = (DRONE_ID_uatype_t)(packet[1] & 0x0F);
	basicID->IDType = (DRONE_ID_idtype_t)((packet[1] >> 4) & 0x0F);
	memcpy(basicID->UASID, &packet[2], DRONE_ID_SIZE);
	basicID->UASID[DRONE_ID_SIZE] = '\0';
	return 0;
}

int decode_location_vector(const uint8_t *packet, int packet_len, DRONE_ID_Location_data *location_data){
    if (packet == NULL || location_data == NULL){
        return DECODE_ERR_NULL;
    }
    if (packet_len < 25){
        return DECODE_ERR_TOO_SHORT;
    }
    location_data->Status = (packet[1] >> 4) & 0x0F;
    location_data->HeightType = (packet[1] >> 2) & 0x01;
    location_data->Direction = packet[2] + ((packet[1] >> 1) & 0x01) * 180;
    location_data->SpeedHorizontal = (packet[1] & 0x01) ? (packet[3] * 0.75 + 63.75) : (packet[3] * 0.25);
    int8_t speed_vertical_raw = (int8_t)packet[4];
    location_data->SpeedVertical = speed_vertical_raw * 0.5;
    location_data->Latitude = rd_i32le(&packet[5]) / 1e7;
    location_data->Longitude = rd_i32le(&packet[9]) / 1e7;
    location_data->AltitudeBaro = rd_u16le(&packet[13]) * 0.5 - 1000; 
    location_data->AltitudeGeo = rd_u16le(&packet[15]) * 0.5 - 1000; 
    location_data->Height = rd_u16le(&packet[17]) * 0.5 - 1000; 
    location_data->HorizAccuracy = packet[19] & 0x0F;
    location_data->VertAccuracy = (packet[19] >> 4) & 0x0F;
    location_data->SpeedAccuracy = packet[20] & 0x0F;
    location_data->BaroAccuracy = (packet[20] >> 4) & 0x0F;
    uint16_t timestamp_raw =  rd_u16le(&packet[21]);
    if (timestamp_raw != INV_TIMESTAMP){
        location_data->TimeStamp = timestamp_raw / 10.f;
    } else {
        location_data->TimeStamp = -1.0f;
    }
    location_data->TSAccuracy = packet[23] & 0x0F;
    return 0;
}

const char *idtype_to_str(DRONE_ID_idtype_t type){
    switch (type){
        case DRONE_ID_IDTYPE_SERIAL_NUMBER:
            return "IDTYPE_SERIAL_NUMBER";
        case DRONE_ID_IDTYPE_CAA_REGISTRATION_ID:
            return "IDTYPE_CAA_REGISTRATION_ID";
        case DRONE_ID_IDTYPE_UTM_ASSIGNED_UUID:
            return "IDTYPE_UTM_ASSIGNED_UUID";
        case DRONE_ID_IDTYPE_SPECIFIC_SESSION_ID:
            return "IDTYPE_SPECIFIC_SESSION_ID";
        case DRONE_ID_IDTYPE_NONE:
            return "IDTYPE_NONE";
        default:
            return "IDTYPE_UNKNOWN";
    }
}

const char *uatype_to_str(DRONE_ID_uatype_t type){
    switch (type){
        case DRONE_ID_UATYPE_AEROPLANE:
            return "UATYPE_AEROPLANE";
        case DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR:
            return "UATYPE_HELICOPTER_OR_MULTIROTOR";
        case DRONE_ID_UATYPE_GYROPLANE:
            return "UATYPE_GYROPLANE";
        case DRONE_ID_UATYPE_HYBRID_LIFT:
            return "UATYPE_HYBRID_LIFT";
        case DRONE_ID_UATYPE_ORNITHOPTER:
            return "UATYPE_ORNITHOPTER";
        case DRONE_ID_UATYPE_GLIDER:
            return "UATYPE_GLIDER";
        case DRONE_ID_UATYPE_KITE:
            return "UATYPE_KITE";
        case DRONE_ID_UATYPE_FREE_BALLOON:
            return "UATYPE_FREE_BALLOON";
        case DRONE_ID_UATYPE_CAPTIVE_BALLOON:
            return "UATYPE_CAPTIVE_BALLOON";
        case DRONE_ID_UATYPE_AIRSHIP:
            return "UATYPE_AIRSHIP";
        case DRONE_ID_UATYPE_FREE_FALL_PARACHUTE:
            return "UATYPE_FREE_FALL_PARACHUTE";
        case DRONE_ID_UATYPE_ROCKET:
            return "UATYPE_ROCKET";
        case DRONE_ID_UATYPE_TETHERED_POWERED_AIRCRAFT:
            return "UATYPE_TETHERED_POWERED_AIRCRAFT";
        case DRONE_ID_UATYPE_GROUND_OBSTACLE:
            return "UATYPE_GROUND_OBSTACLE";
        case DRONE_ID_UATYPE_OTHER:
            return "UATYPE_OTHER";
        default:
            return "UATYPE_UNKNOWN";
    }
}

const char *msg_type_to_str(messagetype_t msg_type){
    switch(msg_type){
        case MESSAGETYPE_BASIC_ID:
            return "MESSAGETYPE_BASIC_ID";
        case MESSAGETYPE_AUTH:
            return "MESSAGETYPE_AUTH";
        case MESSAGETYPE_LOCATION:
            return "MESSAGETYPE_LOCATION";
        case MESSAGETYPE_OPERATOR_ID:
            return "MESSAGETYPE_OPERATOR_ID";
        case MESSAGETYPE_PACKED:
            return "MESSAGETYPE_PACKED";
        case MESSAGETYPE_SYSTEM:
            return "MESSAGETYPE_SYSTEM";
        default:
            return "MESSAGETYPE_INVALID";
    }
}

const char *status_to_str(DRONE_ID_status_t status_type){
    switch(status_type){
        case DRONE_ID_STATUS_GROUND:
            return "STATUS_GROUND";
        case DRONE_ID_STATUS_AIRBORNE:
            return "STATUS_AIRBORNE";
        case DRONE_ID_STATUS_EMERGENCY:
            return "STATUS_EMERGENCY";
        case DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE:
            return "STATUS_REMOTE_ID_SYSTEM_FAILURE";
        case DRONE_ID_STATUS_UNDECLARED:
            return "STATUS_UNDECLARED";
        default:
            return "UNKNOWN_STATUS";
    }
}

const char *auth_type_to_str(auth_type_t auth_type){
    switch(auth_type){
        case AUTH_UAS_ID_SIGNATURE:
            return "AUTH_UAS_ID_SIGNATURE";
        case AUTH_OPERATOR_ID_SIGNATURE:
            return "AUTH_OPERATOR_ID_SIGNATURE";
        case AUTH_MSG_SET_SIGNATURE:
            return "AUTH_MSG_SET_SIGNATURE";
        case AUTH_NET_REMOTE_ID:
            return "AUTH_NET_REMOTE_ID";
        case AUTH_SPECIFIC_AUTH:
            return "AUTH_SPECIFIC_AUTH";
        case AUTH_NONE:
            return "AUTH_NONE";
        default:
            return "UNKNOWN_AUTH";
    }

}

int decode_auth(const uint8_t *packet, int packet_len, authentication_data *auth_data){
    if (packet == NULL || auth_data == NULL){
        return DECODE_ERR_NULL;
    }
    if (packet_len < AUTH_PACK_SIZE){
        return DECODE_ERR_TOO_SHORT;
    }
    auth_data->msg_type = (packet[0] >> 4) & 0x0F;
    auth_data->auth_type = (packet[1] >> 4) & 0x0F;
    auth_data->data_page = packet[1] & 0x0F;
    if (auth_data->data_page == 0){
        auth_data->last_page_idx = packet[2];
        auth_data->length = packet[3];
        auth_data->timestamp = rd_u32le(&packet[4]);
        memcpy(auth_data->AuthData, &packet[8], 17);
    } else {
        memcpy(auth_data->AuthData, &packet[2], 23);
    }
    return 0;
}

int decode_selfID(const uint8_t *packet, int packet_len, selfID_data *selfID){
    if (packet == NULL || selfID == NULL){
        return DECODE_ERR_NULL;
    } 
    if (packet_len < SELFID_PACK_SIZE){
        return DECODE_ERR_TOO_SHORT;
    }
    selfID->msg_type = (packet[0] >> 4) & 0x0F;
    selfID->DescType = packet[1];
    memcpy(selfID->desc, &packet[2], 23);
    selfID->desc[23] = '\0';
    return 0;
}

int decode_system_msg(const uint8_t *packet, int packet_len, sys_data *system_msg){
    if (packet == NULL || system_msg == NULL) {
        return DECODE_ERR_NULL;
    }
    if (packet_len < SYSMSG_PACK_SIZE) {
        return DECODE_ERR_TOO_SHORT;
    }
    system_msg->msg_type            = (packet[0] >> 4) & 0x0F;
    system_msg->OperatorLocType     =  packet[1] & 0x03;          // byte 1, bits 0-1
    system_msg->ClassificationType  = (packet[1] >> 2) & 0x07;    // byte 1, bits 2-4
    system_msg->OperatorLatitude    = rd_i32le(&packet[2]) / 1e7;
    system_msg->OperatorLongitude   = rd_i32le(&packet[6]) / 1e7;
    system_msg->AreaCount           = rd_u16le(&packet[10]);
    system_msg->AreaRadius          = packet[12] * 10;            // encoded /10
    system_msg->AreaCeiling         = rd_u16le(&packet[13]) * 0.5f - 1000.0f;
    system_msg->AreaFloor           = rd_u16le(&packet[15]) * 0.5f - 1000.0f;
    system_msg->ClassEU             =  packet[17] & 0x0F;         // byte 17 low nibble
    system_msg->CategoryEU          = (packet[17] >> 4) & 0x0F;   // byte 17 high nibble
    system_msg->OperatorAltitudeGeo = rd_u16le(&packet[18]) * 0.5f - 1000.0f;
    system_msg->Timestamp           = rd_u32le(&packet[20]);
    return 0;
}

int decode_operator_id(const uint8_t *packet, int packet_len, OperatorID_data *op_id){
    if (packet == NULL || op_id == NULL){
        return DECODE_ERR_NULL;
    }
    if (packet_len < OPID_PACK_SIZE){
        return DECODE_ERR_TOO_SHORT;
    }
    op_id->msg_type = (packet[0] >> 4) & 0x0F;
    op_id->OperatorIdType = packet[1];
     memcpy(op_id->OperatorId, &packet[2], 20);
    op_id->OperatorId[20] = '\0';
    return 0;
}
