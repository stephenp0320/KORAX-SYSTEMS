#include <stdio.h>
#include <stdint.h>
#include "korax/raven_decode.h"
#define MAX_ODID_MSG_SIZE 4096

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size){
    if (size > MAX_ODID_MSG_SIZE){
        return 0;
    }
    decoded_message_t out;
    decode_message(data, (int)size, &out);
    return 0;
}
