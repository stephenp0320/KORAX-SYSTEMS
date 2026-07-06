#include <stdio.h>
#include <stdint.h>
#include "remoteid_decoder.h"
droneID_basicID_data basicID;

int main(void){
    uint8_t packet[] = {
        0x00,       // message type
        0x21,       // IDType = 2, UAType = 1
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X'
    };
    droneID_basicID_data basicID;
    int result = decode_basic_id(packet, sizeof(packet), &basicID);
    if (result != 0) {
        printf("Failed to decode Basic ID\n");
        return 1;
    }
    const char *ua_type = uatype_to_string(basicID.UAType);
    const char *id_type = idtype_to_string(basicID.IDType);
    printf("UA Type: %s\n", ua_type);
    printf("ID Type: %s\n", id_type);
    printf("UAS ID : %s\n", basicID.UASID);
    return 0;
}
