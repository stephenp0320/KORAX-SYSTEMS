#include "korax/capture.h"
#include <string.h>
#include "korax/raven_decode.h"
#include <stdbool.h>

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
            return 0;
        }
        pos += len + 1;
    }
    return BLE_ADV_ERR;
}

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
                return 0;
            }
            return WIFI_ERR;
        }
        pos += 2 + len;
    }
    return WIFI_ERR;
}
