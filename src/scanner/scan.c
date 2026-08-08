#include "korax/scanner.h"

#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int scan(void)
{
    inquiry_info *inq_info = NULL;
    int max_responces, num_responces;
    int device_id, sock, len, flags;
    char address[MAC_ADDR] = { 0 };
    char name[248] = { 0 };

    device_id = hci_get_route(NULL);
    sock = hci_open_dev(device_id);
    if (device_id < 0 || sock < 0) return SCAN_ERR;

    len = INQ_DURATION;
    max_responces = MAX_RESP;
    flags = IREQ_CACHE_FLUSH;
    inq_info = (inquiry_info*)malloc(max_responces * sizeof(inquiry_info));
    num_responces = hci_inquiry(device_id, len, max_responces, NULL, &inq_info, flags);
    if (num_responces < 0) {
        free(inq_info);
        close(sock);
        return RESPONCE_ERR;
    }

    int i;
    for (i = 0; i < num_responces; i++) {
        ba2str(&(inq_info + i)->bdaddr, address);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(inq_info + i)->bdaddr, sizeof(name), name, 0) < 0) {
            strcpy(name, UNKNOWN);
        }
        printf("%s  %s\n", address, name);
    }

    free(inq_info);
    close(sock);

    return SCAN_OK;
}

#else /* !__linux__ */

int scan(void)
{
    return SCAN_ERR;
}

#endif
