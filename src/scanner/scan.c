#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
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

int scan(void)
{
    inquiry_info **inq_info = NULL;
    int max_responces, num_responces;
    int device_id, sock, len, flags;
    char address[MAC_ADDR] = { 0 };
    char name[248] = { 0 };

    device_id = hci_get_route(NULL);
    socket = hci_open_dev(device_id);
    if (device_id < 0 || socket < 0) return SCAN_ERR;

    len = INQ_DURATION;
    max_responces = MAX_RESP;
    flags = IREQ_CACHE_FLUSH;
    inq_info = (inquiry_info*)malloc(max_responces * sizeof(inquiry_info));
    num_responces = hci_inquiry(dev_id, len, max_responces, NULL, &inq_info, flags);
    if (num_responces < 0) return RESPONCE_ERR;
    int i;
    for (i = 0; i < num_responces; i++){
        ba2str(&(inq_info+1)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(inq_info+i)->bdaddr, sizeof(name), name, 0) < 0){
            strcpy(name, UNKNOWN);
            printf("%s  %s\n", addr, name);
        }
    }
    free(inq_info);
    close(socket);

    return SCAN_OK;
}
