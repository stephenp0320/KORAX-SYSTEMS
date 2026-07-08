#include <stdio.h>
#include "capture.h"
#include <string.h>
#include "decode.h"
#include <stdbool.h>
#define BLE_AD_TYPE_SERVICE_DATA_16BIT 0x16

int ble_rid_extraction(const ble_adv_frame_t *ble_frame, uint8_t rid_msg[RID_MSG_SIZE]){
    if (!ble_frame || !rid_msg) {
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
            int payload_start = pos + 4;

            if (payload_start + RID_MSG_SIZE > ble_frame->adv_len) {
                return BLE_ADV_ERR;
            }
            memcpy(rid_msg, &ble_frame->adv_data[payload_start], RID_MSG_SIZE);
            return 0;
        }
        pos += len + 1;
    }
    return BLE_ADV_ERR;
}


