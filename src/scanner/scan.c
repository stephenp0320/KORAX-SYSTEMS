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

int hci_scan_open(void)
{
    int device_id = hci_get_route(NULL);
    int sock = hci_open_dev(device_id);
    if (device_id < 0 || sock < 0) return SCAN_ERR;

    if (hci_le_set_scan_parameters(sock, 0x00, htobs(0x0010), htobs(0x0010),
                                    LE_PUBLIC_ADDRESS, 0x00, 10000) < 0){
        close(sock);
        return SCAN_ERR;
    }
    if (hci_le_set_scan_enable(sock, 0x01, 0x00, 10000) < 0){
        close(sock);
        return SCAN_ERR;
    }

    struct hci_filter new_filter;
    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
    if (setsockopt(sock, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0){
        hci_le_set_scan_enable(sock, 0x00, 0x00, 10000);
        close(sock);
        return SCAN_ERR;
    }

    return sock;
}

int hci_scan_read(int sock, track_table_t *table, time_t now, const magpie_thresholds_t *th, const magpie_profile_t *profile, int *updated_idx)
{
    if (updated_idx) *updated_idx = -1;

    unsigned char event_buf[HCI_MAX_EVENT_SIZE];
    int len = read(sock, event_buf, sizeof(event_buf));
    if (len < 0){
        if (errno == EINTR || errno == EAGAIN) return SCAN_OK;
        return SCAN_ERR;
    }
    if (len < 1 + HCI_EVENT_HDR_SIZE + EVT_LE_META_EVENT_SIZE) return SCAN_OK;

    evt_le_meta_event *meta = (evt_le_meta_event *)(event_buf + 1 + HCI_EVENT_HDR_SIZE);
    if (meta->subevent != EVT_LE_ADVERTISING_REPORT) return SCAN_OK;

    le_advertising_info *info = (le_advertising_info *)(meta->data + 1);
    if (info->length == 0 || info->length > BLE_ADV_MAX_SIZE) return SCAN_OK;

    ble_adv_frame_t frame = { 0 };
    memcpy(frame.adv_data, info->data, info->length);
    frame.adv_len = info->length;
    memcpy(frame.source_mac, info->bdaddr.b, sizeof(frame.source_mac));
    frame.rssi_dbm = (int8_t)info->data[info->length];
    frame.timestamp = (uint64_t)now;

    int idx = process_ble_frame(&frame, now, table, th, profile);   /* -1 just means "not a RID advert" */
    if (updated_idx) *updated_idx = idx;
    return SCAN_OK;
}

void hci_scan_close(int sock)
{
    if (sock < 0) return;
    hci_le_set_scan_enable(sock, 0x00, 0x00, 10000);
    close(sock);
}

#else /* !__linux__ */

int hci_scan_open(void)
{
    return SCAN_ERR;
}

int hci_scan_read(int sock, track_table_t *table, time_t now, const magpie_thresholds_t *th, const magpie_profile_t *profile, int *updated_idx)
{
    (void)sock; (void)table; (void)now; (void)th; (void)profile;
    if (updated_idx) *updated_idx = -1;
    return SCAN_ERR;
}

void hci_scan_close(int sock)
{
    (void)sock;
}

#endif
