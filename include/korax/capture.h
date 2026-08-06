/*
 * Korax — capture-layer public API.
 *
 * This header is the interchangeable-source contract: live BLE capture
 * (capture_ble.c, Linux-only) and file replay (capture_file.c, portable)
 * both produce the same frame/message shapes and feed the exact same
 * pipeline entry points downstream. That's why the rest of the system
 * develops and tests fine on macOS despite BLE capture being Linux-only.
 */
#ifndef CAPTURE_H
#define CAPTURE_H
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include "korax/raven_decode.h"
#define BLE_ADV_MAX_SIZE 31
#define BLE_EXTRACTION_SUCCESS 0
#define BLE_ADV_ERR 1
#define BLE_AD_TYPE_SERVICE_DATA_16BIT 0x16
#define WIFI_MAX_PACKET_SIZE 2048
#define WIFI_EXTRACTION_SUCCESS 0
#define WIFI_ERR 1
#define ODID_SERVICE_UUID 0xFFFA
#define ODID_APP_CODE 0x0D

/* One raw BLE advertisement as seen over the air. source_mac is what the
 * pipeline keys tracks on for the live-capture path. */
typedef struct ble_adv_frame_t {
	uint8_t adv_data[BLE_ADV_MAX_SIZE];
	uint8_t adv_len;
	uint8_t source_mac[6];
	int8_t rssi_dbm;
	uint64_t timestamp;
} ble_adv_frame_t;

/* Which radio medium/mode a wp_frame_t came from; WP_MEDIUM_REPLAY marks
 * frames sourced from capture_file.c rather than a live radio. */
typedef enum {
    WP_MEDIUM_BLE_LEGACY,
    WP_MEDIUM_BLE_EXTENDED,
    WP_MEDIUM_WIFI_BEACON,
    WP_MEDIUM_WIFI_NAN,
    WP_MEDIUM_REPLAY,
} wp_medium_t;

/* Generic frame envelope spanning all capture media (BLE, Wi-Fi, replay). */
typedef struct wp_frame_t {
	const uint8_t *payload;
	size_t payload_len;
	uint8_t source_mac[6];
	wp_medium_t medium;
	int8_t rssi;
	uint64_t timestamp;
} wp_frame_t;

typedef void (*wp_frame_cb_t)(
    const wp_frame_t *frame,
    void *user_data
);

typedef struct wifi_frame_t {
	uint8_t wifi_packet[WIFI_MAX_PACKET_SIZE];
	int packet_len;
	uint8_t source_mac[6];
    	int8_t rssi_dbm;
    	uint64_t timestamp;
} wifi_frame_t;

/* Pulls the 25-byte ODID payload + message counter out of a raw BLE
 * advertisement. Returns 0 on success, BLE_ADV_ERR if no matching ODID
 * service-data AD structure is found or the frame is malformed. */
int ble_rid_extraction(const ble_adv_frame_t *ble_frame, uint8_t rid_msg[RID_MSG_SIZE], uint8_t *msg_counter);

/* Drives the file-replay source: reads `path` line by line and invokes `cb`
 * once per frame. Returns the frame count, or -1 on error. */
int wp_replay_run(const char *path, wp_frame_cb_t cb, void *user_data);
FILE *read_file(const char *filename);
int get_file_line(FILE *fp, wp_frame_cb_t cb, void *user_data);
#endif
