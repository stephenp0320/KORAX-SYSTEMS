#ifndef SCANNER_H
#define SCANNER_H
#include <stdint.h>
#define MAX_RESP 255
// defined mac address here, needs further configs
#define MAC_ADDR 19
#define SCAN_ERR -1
#define RESPONCE_ERR -1
#define SCAN_OK 0
#define UNKNOWN "unknown"
#define INQ_DURATION 8

/* Classic BR/EDR inquiry scan smoke test. Linux-only (BlueZ); returns
 * SCAN_ERR on any other platform. bdaddr_t/inquiry_info come from BlueZ's
 * own headers on Linux -- not redeclared here to avoid conflicting with them. */
int scan(void);
#endif
