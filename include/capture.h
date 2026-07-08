#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "decode.h"
#define BLE_ADV_MAX_SIZE 31
#define RID_MSG_SIZE 25
#define BLE_ADV_ERR 1
#define TRACK_ERR 1 
#define TRACK_NOT_FOUND -1
#define TRACK_TABLE_FULL -2
#define MAX_TRACKS 100

typedef struct ble_adv_frame_t {
	uint8_t adv_data[BLE_ADV_MAX_SIZE];
	uint8_t adv_len;
	uint8_t source_mac[6];
	int8_t rssi_dbm;
	uint64_t timestamp;
} ble_adv_frame_t;

typedef struct {
    char uas_id[21];
    droneID_basicID_data basic_id;
    DRONE_ID_Location_data location;
    sys_data system;
    OperatorID_data operator_id;
    selfID_data self_id;
    authentication_data auth;
    int has_basic_id;
    int has_auth;
    int has_location;
    int has_system;
    int has_operator_id;
    int has_self_id;
    int msg_count_basic_id;
    int msg_count_location;
    int msg_count_system;
    int msg_count_operator_id;
    time_t first_seen;
    time_t last_seen;
} track_t;

typedef struct {
    track_t tracks[MAX_TRACKS];
    int num_tracks;
} track_table_t;
