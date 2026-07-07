#include <stdio.h>
#include <stdint.h>
#include "decode.h"

static void print_decoded_message(const decoded_message_t *msg){
    switch (msg->type) {
        case MESSAGETYPE_BASIC_ID:
            printf("===== Basic ID =====\n");
            printf("UA Type: %s\n", uatype_to_str(msg->data.basic_id.UAType));
            printf("ID Type: %s\n", idtype_to_str(msg->data.basic_id.IDType));
            printf("UAS ID : %s\n\n", msg->data.basic_id.UASID);
            break;
        case MESSAGETYPE_LOCATION:
            printf("===== Location =====\n");
            printf("Status   : %s\n", status_to_str(msg->data.location.Status));
            printf("Latitude : %.7f\n", msg->data.location.Latitude);
            printf("Longitude: %.7f\n", msg->data.location.Longitude);
            printf("Altitude : %.1f m\n", msg->data.location.AltitudeGeo);
            printf("Timestamp: %.1f s\n\n", msg->data.location.TimeStamp);
            break;
        case MESSAGETYPE_AUTH:
            printf("===== Authentication =====\n");
            printf("Data Page : %u\n", msg->data.auth.data_page);
            printf("Length    : %u bytes\n", msg->data.auth.length);
            printf("Timestamp : %u\n\n", msg->data.auth.timestamp);
            break;
        case MESSAGETYPE_SELF_ID:
            printf("===== Self ID =====\n");
            printf("Description Type: %u\n", msg->data.self_id.DescType);
            printf("Description     : %s\n\n", msg->data.self_id.desc);
            break;
        case MESSAGETYPE_SYSTEM:
            printf("===== System =====\n");
            printf("Operator Latitude : %.7f\n", msg->data.system.OperatorLatitude);
            printf("Operator Longitude: %.7f\n", msg->data.system.OperatorLongitude);
            printf("Area Radius       : %u m\n", msg->data.system.AreaRadius);
            printf("Area Ceiling      : %.1f m\n\n", msg->data.system.AreaCeiling);
            break;
        case MESSAGETYPE_OPERATOR_ID:
            printf("===== Operator ID =====\n");
            printf("Operator ID Type: %u\n", msg->data.operator_id.OperatorIdType);
            printf("Operator ID     : %s\n\n", msg->data.operator_id.OperatorId);
            break;
        default:
            printf("Unknown or unsupported message\n\n");
            break;
    }
}

int main(void){
    uint8_t packets[][25] = {
        {
            0x00, 0x21,
            '1','5','8','1','F','5','B','J','D','2',
            '4','8','Q','0','0','A','8','X','V','X',
            0x00, 0x00, 0x00
        },
        {
            0x01, 0x00, 0x5A, 0x14, 0x82,
            0x1F, 0x65, 0x4A, 0x80,
            0xFA, 0xDB, 0x8E, 0x80,
            0x08, 0x98, 0x08, 0x98,
            0x3C, 0x0B, 0x06, 0x06, 0x04, 0x02,
            0x05, 0xDC
        },
        {
            0x03, 0x01,
            'I','n','s','p','e','c','t','i','o','n',' ',
            'F','l','i','g','h','t',
            0,0,0,0,0,0
        }
    };

    for (int i = 0; i < 3; i++) {
        decoded_message_t msg;
        int res = decode_message(packets[i], 25, &msg);
        if (res != 0) {
            printf("Failed to decode packet %d\n", i);
            continue;
        }
        print_decoded_message(&msg);
    }
    return 0;
}
