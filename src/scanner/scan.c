#include "korax/scanner.h"

#ifdef __linux__
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "korax/capture.h"
#include "korax/pipeline.h"     /* process_ble_frame */
#include "korax/rook_track.h"

int scan(track_table_t *table)
{
    int device_id, sock;
    struct hci_filter new_filter, old_filter;
    socklen_t old_filter_len;
    unsigned char event_buf[HCI_MAX_EVENT_SIZE];

    device_id = hci_get_route(NULL);
    sock = hci_open_dev(device_id);
    if (device_id < 0 || sock < 0) return SCAN_ERR;

    int responce = hci_le_set_scan_parameters(sock, 0x00, htobs(0x0010), htobs(0x0010),
                                              LE_PUBLIC_ADDRESS, 0x00, 10000);
    if (responce < 0){ close(sock); return RESPONCE_ERR; }

    int enable = hci_le_set_scan_enable(sock, 0x01, 0x00, 10000);
    if (enable < 0){ close(sock); return SCAN_ERR; }

    old_filter_len = sizeof(old_filter);
    if (getsockopt(sock, SOL_HCI, HCI_FILTER, &old_filter, &old_filter_len) < 0){
        hci_le_set_scan_enable(sock, 0x00, 0x00, 10000);
        close(sock);
        return SCAN_ERR;
    }

    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
    if (setsockopt(sock, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0){
        hci_le_set_scan_enable(sock, 0x00, 0x00, 10000);
        close(sock);
        return SCAN_ERR;
    }

    bool running = true;
    while (running){
        int len = read(sock, event_buf, sizeof(event_buf));
        if (len < 0){
            if (errno == EINTR || errno == EAGAIN) continue;
            break;
        }
        if (len < 1 + HCI_EVENT_HDR_SIZE + EVT_LE_META_EVENT_SIZE) continue;

        evt_le_meta_event *meta = (evt_le_meta_event *)(event_buf + 1 + HCI_EVENT_HDR_SIZE);
        if (meta->subevent != EVT_LE_ADVERTISING_REPORT) continue;

        le_advertising_info *info = (le_advertising_info *)(meta->data + 1);
        if (info->length == 0 || info->length > BLE_ADV_MAX_SIZE) continue;

        ble_adv_frame_t frame = { 0 };
        memcpy(frame.adv_data, info->data, info->length);
        frame.adv_len = info->length;
        memcpy(frame.source_mac, info->bdaddr.b, sizeof(frame.source_mac));
        frame.rssi_dbm = (int8_t)info->data[info->length];
        frame.timestamp = (uint64_t)time(NULL);

        time_t now = time(NULL);
        process_ble_frame(&frame, now, table);   /* -1 just means "not a RID advert" — keep scanning */
    }

    hci_le_set_scan_enable(sock, 0x00, 0x00, 10000);
    setsockopt(sock, SOL_HCI, HCI_FILTER, &old_filter, sizeof(old_filter));
    close(sock);
    return SCAN_OK;
}

#else /* !__linux__ */

int scan(track_table_t *table)
{
    (void)table;
    return SCAN_ERR;
}

#endif
