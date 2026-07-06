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
    printf("UA Type: %s\n", uatype_to_string(basicID.UAType));
    printf("ID Type: %s\n", idtype_to_string(basicID.IDType));
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
    printf("Latitude : %.7f\n", location.Latitude);
    printf("Longitude: %.7f\n", location.Longitude);
    printf("Altitude : %.1f m\n", location.AltitudeGeo);
    printf("Timestamp: %.1f s\n", location.TimeStamp);
    return 0;
}
