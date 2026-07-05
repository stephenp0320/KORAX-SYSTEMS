#include <stdio.h>
#include <string.h>
#define openDroneIDErr 1
#define DJIMAXSIZE 20
#define DJI 4;
int decoder(char *openDroneID){
	if (!openDroneID){
		return openDroneIDErr;
	}
	char man_code[DJIMAXSIZE];
	int DJI_ch_num = 4;
	char *drone_id = strncpy(man_code, openDroneID, DJI_ch_num);
	man_code[DJI_ch_num] = '\0';
	printf("DRONE_ID: %s\n", drone_id);
	return 0;
}
