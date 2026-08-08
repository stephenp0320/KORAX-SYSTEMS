#ifndef SCANNER_H
#include <stdint.h>
#define MAX_RESP 255
// defined mac address here, needs further configs
#define MAC_ADDR 19
#define SCAN_ERR -1
#define RESPONCE_ERR -1
#define SCAN_OK 0
#define UNKNOWN "unknown"
#define INQ_DURATION 8

typedef struct {
	uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

typedef struct {
    bdaddr_t    bdaddr;
    uint8_t     pscan_rep_mode;
    uint8_t     pscan_period_mode;
    uint8_t     pscan_mode;
    uint8_t     dev_class[3];
    uint16_t    clock_offset;
} __attribute__ ((packed)) inquiry_info;
#endif
