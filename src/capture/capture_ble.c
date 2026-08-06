/*
 * Korax — BLE capture: raw advertisement -> 25-byte ODID payload.
 *
 * This is the live-radio side of the interchangeable source contract (see
 * capture.h); its counterpart is capture_file.c for replay. Pure buffer
 * parsing only — no sockets/BlueZ here, so it builds and tests on any OS
 * even though live BLE capture itself is Linux-only.
 */
#include "korax/capture.h"
#include <string.h>
#include "korax/raven_decode.h"
#include <stdbool.h>

/*
 * Walks the BLE AD (advertising data) structures looking for the
 * Service Data (16-bit UUID) entry matching the ODID service UUID/app code,
 * per ASTM F3411 BLE legacy advertising. Extracts the message counter and
 * copies out the RID_MSG_SIZE payload. Returns 0 on success, BLE_ADV_ERR on
 * a malformed frame or if no matching AD structure is present.
 * `msg_counter` may be NULL if the caller doesn't need it.
 */
int ble_rid_extraction(const ble_adv_frame_t *ble_frame, uint8_t rid_msg[RID_MSG_SIZE], uint8_t *msg_counter){
    if (ble_frame == NULL || rid_msg == NULL) {
        return BLE_ADV_ERR;
    }
    int pos = 0;
    while (pos < ble_frame->adv_len) {
        uint8_t len = ble_frame->adv_data[pos];
        if (len == 0) {
            break;
        }
        if (pos + len >= ble_frame->adv_len) {
            return BLE_ADV_ERR;
        }
        uint8_t type = ble_frame->adv_data[pos + 1];
        if (type == BLE_AD_TYPE_SERVICE_DATA_16BIT) {
            int payload_start = pos + 6;

            if (payload_start + RID_MSG_SIZE > ble_frame->adv_len) {
                return BLE_ADV_ERR;
            }

            uint16_t uuid = ble_frame->adv_data[pos + 2] | (ble_frame->adv_data[pos + 3] << 8);
            uint8_t app_code = ble_frame->adv_data[pos + 4];

            if (uuid != ODID_SERVICE_UUID || app_code != ODID_APP_CODE) {
                pos += len + 1;
                continue;
            }
            uint8_t counter = ble_frame->adv_data[pos + 5];
            if (msg_counter){
                *msg_counter = counter;
            }

            memcpy(rid_msg, &ble_frame->adv_data[payload_start], RID_MSG_SIZE);
            return BLE_EXTRACTION_SUCCESS;
        }
        pos += len + 1;
    }
    return BLE_ADV_ERR;
}

/* Wi-Fi counterpart to ble_rid_extraction(): scans vendor-specific
 * information elements (element ID 0xDD) for the ODID payload, skipping the
 * 3-byte OUI/type prefix. Returns 0 on success, WIFI_ERR otherwise. */
int wifi_rid_extraction(const wifi_frame_t *wifi_frame, uint8_t rid_msg[RID_MSG_SIZE]){
    if (wifi_frame == NULL || rid_msg == NULL){
        return WIFI_ERR;
    }
    int pos = 0;
    while (pos + 2 <= wifi_frame->packet_len){
        uint8_t element_id = wifi_frame->wifi_packet[pos];
        uint8_t len = wifi_frame->wifi_packet[pos+1];

        if (element_id == 0xDD){
            if (len >= 3 + RID_MSG_SIZE){
                int payload_start = pos + 2 + 3;
                if (payload_start + RID_MSG_SIZE > wifi_frame->packet_len){
                    return WIFI_ERR;
                }
                memcpy(rid_msg, &wifi_frame->wifi_packet[payload_start], RID_MSG_SIZE);
                return WIFI_EXTRACTION_SUCCESS;
            }
            return WIFI_ERR;
        }
        pos += 2 + len;
    }
    return WIFI_ERR;
}
