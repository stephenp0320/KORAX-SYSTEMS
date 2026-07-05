#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define openDroneIDErr 1
#define DJIMAXSIZE 20
#define DJI 4;
#define WP_ID_MAX_LEN 20
int decoder(char *openDroneID){
	if (!openDroneID){
		return openDroneIDErr;
	}
	char man_code[DJIMAXSIZE];
	int DJI_ch_num = 4;
	int pos = 0;
	
	char *drone_id = strncpy(man_code, openDroneID, DJI_ch_num);
	man_code[DJI_ch_num] = '\0';
	printf("DRONE_ID: %s\n", drone_id);
	
	pos += DJI_ch_num;
	char serial_len[2] = { openDroneID[pos], '\0'};
	unsigned char value = (unsigned char)strtol(serial_len, NULL, 16);
	printf("SERIAL_LENGTH: %d\n", value);
	
	pos += 1;
	char ch_serial[WP_ID_MAX_LEN];
	int j = 0;
	for (int i = pos; i < pos + value && j < WP_ID_MAX_LEN - 1; i++){
		ch_serial[j++] = openDroneID[i];
	}
	ch_serial[j] = '\0';
	printf("CHAR_SERIAL: %s\n", ch_serial);
	return 0;
}
