#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "decode.h"
#include "track_table.h"
#define BLE_ADV_MAX_SIZE 31
#define BLE_ADV_ERR 1
#define BLE_AD_TYPE_SERVICE_DATA_16BIT 0x16

typedef struct ble_adv_frame_t {
	uint8_t adv_data[BLE_ADV_MAX_SIZE];
	uint8_t adv_len;
	uint8_t source_mac[6];
	int8_t rssi_dbm;
	uint64_t timestamp;
} ble_adv_frame_t;

int ble_rid_extraction(const ble_adv_frame_t *ble_frame, uint8_t rid_msg[RID_MSG_SIZE]);
