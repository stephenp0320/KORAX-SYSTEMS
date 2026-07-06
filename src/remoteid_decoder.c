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

const char *idtype_to_string(DRONE_ID_idtype_t type){
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

const char *uatype_to_string(DRONE_ID_uatype_t type){
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

