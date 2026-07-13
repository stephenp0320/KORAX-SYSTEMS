#ifndef CAPTURE_H
#define CAPTURE_H
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include "raven_decode.h"
#define BLE_ADV_MAX_SIZE 31
#define BLE_ADV_ERR 1
#define BLE_AD_TYPE_SERVICE_DATA_16BIT 0x16
#define WIFI_MAX_PACKET_SIZE 2048
#define WIFI_ERR 1

typedef struct ble_adv_frame_t {
	uint8_t adv_data[BLE_ADV_MAX_SIZE];
	uint8_t adv_len;
	uint8_t source_mac[6];
	int8_t rssi_dbm;
	uint64_t timestamp;
} ble_adv_frame_t;

typedef enum {
    WP_MEDIUM_BLE_LEGACY,
    WP_MEDIUM_BLE_EXTENDED,
    WP_MEDIUM_WIFI_BEACON,
    WP_MEDIUM_WIFI_NAN,
    WP_MEDIUM_REPLAY,
} wp_medium_t;

typedef struct wp_frame_t {
	const uint8_t *payload;
	size_t payload_len;
	uint8_t source_mac[6];
	wp_medium_t medium;
	int8_t rssi; 
	uint64_t timestamp;
} wp_frame_t;

typedef void (*wp_frame_cb_t)(const wp_frame_t *frame, void *user_data);

typedef struct wifi_frame_t {
	uint8_t wifi_packet[WIFI_MAX_PACKET_SIZE];
	int packet_len;
	uint8_t source_mac[6];
    	int8_t rssi_dbm;
    	uint64_t timestamp;
} wifi_frame_t;

int ble_rid_extraction(const ble_adv_frame_t *ble_frame, uint8_t rid_msg[RID_MSG_SIZE]);
int wp_replay_run(const char *path, wp_frame_cb_t cb, void *user_data);
#endif
