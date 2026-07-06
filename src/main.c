#include <stdio.h>
#include <stdint.h>
#include "remoteid_decoder.h"
droneID_basicID_data basicID;

int main(void){
    uint8_t packet[] = {
        0x00,       // message type
        0x00,       // protocol/version/reserved
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
    printf("UA Type: %d\n", basicID.UAType);
    printf("ID Type: %d\n", basicID.IDType);
    printf("UAS ID : %s\n", basicID.UASID);
    return 0;
}
