#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define openDroneIDErr 1
#define DJIMAXSIZE 20
#define DJI 4;
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

	return 0;
}
