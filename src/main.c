#include <stdio.h>
#include <stdint.h>
#include "remoteid_decoder.h"
int main(void){
    uint8_t basic_packet[] = {
        0x00,       // message type/version
        0x21,       // IDType = 2, UAType = 1
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X'
    };
    droneID_basicID_data basicID;
    int basic_result = decode_basic_id(basic_packet, sizeof(basic_packet), &basicID);
    if (basic_result != 0) {
        printf("Failed to decode Basic ID\n");
        return 1;
    }

    printf("===== Basic ID =====\n");
    printf("UA Type: %s\n", uatype_to_str(basicID.UAType));
    printf("ID Type: %s\n", idtype_to_str(basicID.IDType));
    printf("UAS ID : %s\n\n", basicID.UASID);
    uint8_t location_packet[] = {
        0x10, 0x20, 0x5A, 0x14, 0x82,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x07, 0xD0,
        0x08, 0x98,
        0x3C,
        0x0B, 0x06, 0x06, 0x04, 0x02,
        0x05, 0xDC
    };
    DRONE_ID_Location_data location;
    int location_result = decode_location_vector(location_packet, sizeof(location_packet), &location);
    if (location_result != 0) {
        printf("Failed to decode Location Vector\n");
        return 1;
    }

    printf("===== Location Vector =====\n");
    printf("Status   : %s\n", status_to_str(location.Status));
    printf("Latitude : %.7f\n", location.Latitude);
    printf("Longitude: %.7f\n", location.Longitude);
    printf("Altitude : %.1f m\n", location.AltitudeGeo);
    printf("Timestamp: %.1f s\n\n", location.TimeStamp);

    authentication_data auth_data;
    uint8_t auth_packet[] = {
        0x20,       // Message Type (Authentication)
        0x01,       // Authentication Type
        0x00,       // Data Page = 0
        0x02,       // Last Page Index = 2
        0x3F,       // Total Authentication Length = 63 bytes
        0x00, 0x01, 0xE2, 0x40,   // Timestamp = 123456
        // Authentication Data (16 bytes)
        0xA1, 0xB2, 0xC3, 0xD4,
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC
};
    uint8_t auth_packet_page1[] = {
        0x20,
        0x01,
        0x01,
        0xDD, 0xEE, 0xFF, 0x10,
        0x20, 0x30, 0x40, 0x50,
        0x60, 0x70, 0x80, 0x90,
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0x01, 0x02
};

    int authentication_result = decode_auth(auth_packet, sizeof(auth_packet), &auth_data);
    if (authentication_result != 0){
        printf("failed to decode authentication");
        return -1;
    }
    printf("===== Authentication =====\n");
    printf("Message Type    : %s\n", msg_type_to_str(auth_data.msg_type));
    printf("Auth Type       : %s\n", auth_type_to_str(auth_data.auth_type));
    printf("Data Page       : %u\n", auth_data.data_page);
    printf("Last Page Index : %u\n", auth_data.last_page_idx);
    printf("Length          : %u bytes\n", auth_data.length);
    printf("Timestamp       : %u\n", auth_data.timestamp);

    printf("Authentication Data:\n");
    int bytes = (auth_data.data_page == 0) ? 16 : 22;
    for (int i = 0; i < bytes; i++){
        printf("%02X ", auth_data.AuthData[i]);

            if ((i + 1) % 8 == 0)
                printf("\n");
    }
    printf("\n");

    selfID_data selfID_data;
    uint8_t self_packet[] = {
        0x30,
        0x01,
        'I','n','s','p','e','c','t','i','o','n',' ',
        'F','l','i','g','h','t',
        0,0,0,0,0,0
};
    int selfID_result = decode_selfID(self_packet, sizeof(self_packet), &selfID_data);
    if (selfID_result != 0){
        printf("failed to decode selfID\n");
        return -1;
    }
    printf("===== Self Identification =====\n");
    printf("Message Type     : %u\n", selfID_data.msg_type);
    printf("Description Type : %u\n", selfID_data.DescType);
    printf("Description      : %s\n\n", selfID_data.desc);

    sys_data sys_msg_data;
    uint8_t system_packet[] = {
    0x40,
    0x1F, 0x65, 0x4A, 0x80,
    0xFA, 0xDB, 0x8E, 0x80,
    0x02,
    0x64,
    0x08, 0x98,
    0x07, 0xF8,
    0x21,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00
};
    int sys_msg_Result = decode_system_msg(system_packet, sizeof(system_packet), &sys_msg_data);
    if (sys_msg_Result != 0){
        printf("failed to decode system message\n");
        return -1;
    }
    printf("===== System Message =====\n");
    printf("Message Type       : %u\n", sys_msg_data.msg_type);
    printf("Operator Loc Type  : %u\n", sys_msg_data.OperatorLocType);
    printf("Classification Type: %u\n", sys_msg_data.ClassificationType);
    printf("Area Count         : %u\n", sys_msg_data.AreaCount);
    printf("Operator Latitude  : %.7f\n", sys_msg_data.OperatorLatitude);
    printf("Operator Longitude : %.7f\n", sys_msg_data.OperatorLongitude);
    printf("Area Radius        : %u m\n", sys_msg_data.AreaRadius);
    printf("Area Ceiling       : %.1f m\n", sys_msg_data.AreaCeiling);
    printf("Area Floor         : %.1f m\n", sys_msg_data.AreaFloor);
    printf("Category (EU)      : %u\n", sys_msg_data.CategoryEU);
    printf("Class (EU)         : %u\n", sys_msg_data.ClassEU);
    printf("Operator Altitude  : %.1f m\n", sys_msg_data.OperatorAltitudeGeo);
    printf("Timestamp          : %u\n\n", sys_msg_data.Timestamp);

    OperatorID_data op_id_data;
    uint8_t operator_packet[] = {
    0x50,
    0x01,
    'I','E','-','O','P','-','1','2','3','4',
    '5','6','7','8','9','A','B','C','D','E',
    0x00, 0x00, 0x00
};
    int operator_id_result = decode_operator_id(operator_packet, sizeof(operator_packet), &op_id_data);
    if (operator_id_result != 0){
        printf("failed to decode operator id\n");
        return -1;
    }
    printf("===== Operator ID =====\n");
    printf("Message Type     : %u\n", op_id_data.msg_type);
    printf("Operator ID Type : %u\n", op_id_data.OperatorIdType);
    printf("Operator ID      : %s\n\n", op_id_data.OperatorId);
    return 0;
}
