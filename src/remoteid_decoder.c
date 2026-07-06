#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "remoteid_decoder.h"
#define DJIMAXSIZE 20
#define DJI 4
#define WP_ID_MAX_LEN 20
//decodes the drones identifer
int decoder(char *droneID){
	if (!droneID){
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

