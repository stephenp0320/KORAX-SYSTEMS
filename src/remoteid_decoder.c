#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "remoteid_decoder.h"
#define DJIMAXSIZE 20
#define DJI 4
#define WP_ID_MAX_LEN 20
//decodes the drones identifer

int decode_basic_id(const uint8_t *packet, int packet_len, droneID_basicID_data *basicID){
	if (!packet || !basicID){
		return DRONE_ID_ERR;
	}
	if (packet_len < 22){
		return DRONE_ID_ERR;
	}
	uint8_t msg_type = (packet[0] >> 4) & 0x0F;
	if (msg_type != 0){
		return DRONE_ID_ERR;
	}
//	uint8_t protocol_version = packet[1];
//	if (protocol_version != 1){
//		return DRONE_ID_ERR;
//	}
	basicID->UAType = (DRONE_ID_uatype_t)packet[1] & 0x0F;
	basicID->IDType = (DRONE_ID_idtype_t)(packet[1] >> 4) & 0x0F; 
	memcpy(basicID->UASID, &packet[2], DRONE_ID_SIZE);
	basicID->UASID[DRONE_ID_SIZE] = '\0';
	return 0;
}

int decode_location_vector(const uint8_t *packet, int packet_len, DRONE_ID_Location_data *location_data){
    if (!packet || !location_data){
        return DRONE_LOCATION_ERR;
    }
    if (packet_len < 25){
        return DRONE_LOCATION_ERR;
    }
    location_data->Status = packet[1] & 0x0F;
    location_data->Direction = packet[2];
    location_data->SpeedHorizontal = packet[3];
    location_data->SpeedVertical = packet[4];
    int32_t latitude_raw = 
        ((int32_t)packet[5] << 24) | 
        ((int32_t)packet[6] << 16) | 
        ((int32_t)packet[7] << 8)  | 
        ((int32_t)packet[8]);
    location_data->Latitude = latitude_raw / 10000000.0;
    int32_t longitude_raw = 
        ((int32_t)packet[9] << 24) | 
        ((int32_t)packet[10] << 16)| 
        ((int32_t)packet[11] << 8) | 
        ((int32_t)packet[12]);
    location_data->Longitude = longitude_raw / 10000000.0;
    uint16_t barometric_alt_raw = 
        ((uint16_t)packet[13] << 8) |
        (uint16_t)packet[14];
    location_data->AltitudeBaro = (barometric_alt_raw * 0.5f) - 1000.0f; 
    uint16_t gedetic_alt_raw = 
        ((uint16_t)packet[15] << 8) |
        (uint16_t)packet[16];
    location_data->AltitudeGeo = (gedetic_alt_raw * 0.5f) - 1000.0f; 
    location_data->Height = packet[17] & 0x0F;
    location_data->HorizAccuracy = packet[18] & 0x0F;
    location_data->VertAccuracy = packet[19] & 0x0F;
    location_data->BaroAccuracy = packet[20] & 0x0F;
    location_data->SpeedAccuracy = packet[21] & 0x0F;
    location_data->TSAccuracy = packet[22] & 0x0F;
    uint16_t timestamp_raw = 
        ((uint16_t)packet[23] << 8) |
         (uint16_t)packet[24];
    if (timestamp_raw != INV_TIMESTAMP){
        location_data->TimeStamp = timestamp_raw / 10.0f;
    } else {
        location_data->TimeStamp = -1.0f;
    }
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
    if (!packet || !auth_data){
        return AUTH_ERR;
    }
    if (packet_len < 25){
        return AUTH_ERR;
    }
    auth_data->msg_type = (packet[0] >> 4) & 0x0F;
    auth_data->auth_type = packet[1] & 0x0F;
    auth_data->data_page = packet[2] & 0x0F;
    if (auth_data->data_page == 0){
        auth_data->last_page_idx = packet[3];
        auth_data->length = packet[4];
        auth_data->timestamp =
            ((uint32_t)packet[5] << 24) |
            ((uint32_t)packet[6] << 16) |
            ((uint32_t)packet[7] << 8)  |
            ((uint32_t)packet[8]);
        memcpy(auth_data->AuthData, &packet[9], 16);
    } else {
        memcpy(auth_data->AuthData, &packet[3], 22);
    }
    return 0;
}

int decode_selfID(const uint8_t *packet, int packet_len, selfID_data *selfID){
    if (!packet || !selfID){
        return DRONE_ID_ERR;
    } 
    if (packet_len < 25){
        return DRONE_ID_ERR;
    }
    selfID->msg_type = packet[0];
    selfID->DescType = packet[1];
    memcpy(selfID->desc, &packet[2], 23);
    selfID->desc[23] = '\0';
    return 0;
}

int decode_system_msg(const uint8_t *packet, int packet_len, sys_data *system_msg){
    if (!packet || !system_msg) {
        return DRONE_ID_ERR;
    }

    if (packet_len < 25) {
        return DRONE_ID_ERR;
    }

    system_msg->msg_type = packet[0];
    system_msg->OperatorLocType = 0;
    system_msg->ClassificationType = 0;

    int32_t operator_lat_raw =
        ((int32_t)packet[1] << 24) |
        ((int32_t)packet[2] << 16) |
        ((int32_t)packet[3] << 8)  |
        ((int32_t)packet[4]);

    system_msg->OperatorLatitude = operator_lat_raw / 10000000.0;
    int32_t operator_lon_raw =
        ((int32_t)packet[5] << 24) |
        ((int32_t)packet[6] << 16) |
        ((int32_t)packet[7] << 8)  |
        ((int32_t)packet[8]);

    system_msg->OperatorLongitude = operator_lon_raw / 10000000.0;
    system_msg->AreaCount = packet[9];
    system_msg->AreaRadius = packet[10];
    uint16_t areaCeil_raw =
        ((uint16_t)packet[11] << 8) |
        (uint16_t)packet[12];

    system_msg->AreaCeiling = (areaCeil_raw * 0.5f) - 1000.0f;
    uint16_t areaFloor_raw =
        ((uint16_t)packet[13] << 8) |
        (uint16_t)packet[14];

    system_msg->AreaFloor = (areaFloor_raw * 0.5f) - 1000.0f;
    system_msg->CategoryEU = packet[15];
    system_msg->ClassEU = 0;
    system_msg->OperatorAltitudeGeo = 0.0f;
    system_msg->Timestamp = 0;
    return 0;
}

int decode_operator_id(const uint8_t *packet, int packet_len, OperatorID_data *op_id){
    if (!packet || !op_id){
        return OPP_ID_ERR;
    }
    if (packet_len < 25){
        return OPP_ID_ERR;
    }
    op_id->msg_type = packet[0];
    op_id->OperatorIdType = packet[1];
     memcpy(op_id->OperatorId, &packet[2], 20);
    op_id->OperatorId[20] = '\0';
    return 0;
}

int decoder(char *droneID, droneID_basicID_data *basicID){
	if (!droneID || !basicID){
		return DRONE_ID_ERR;
	}
	int len = strlen(droneID);
	if (len < DJI + 1){
		return DRONE_ID_ERR;
	}
	char man_code[DJIMAXSIZE];
	int pos = 0;
	
	char *drone_id = strncpy(man_code, droneID, DJI);
	man_code[DJI] = '\0';
	printf("DRONE_ID: %s\n", drone_id);
	
	pos += DJI;
	char serial_len[2] = { droneID[pos], '\0'};
	unsigned char value = (unsigned char)strtol(serial_len, NULL, 16);
	printf("SERIAL_LENGTH: %d\n", value);
	
	pos += 1;
	if (pos + value > len){
		return DRONE_ID_ERR;
	}
	char ch_serial[WP_ID_MAX_LEN];
	int j = 0;
	for (int i = pos; i < pos + value && j < WP_ID_MAX_LEN - 1; i++){
		ch_serial[j++] = droneID[i];
	}
	ch_serial[j] = '\0';
	printf("SERIAL_NUMBER: %s\n", ch_serial);
	return 0;
}

