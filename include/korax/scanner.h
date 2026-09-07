#ifndef SCANNER_H
#define SCANNER_H
#include <stdint.h>
#include <time.h>
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"
#define MAX_RESP 255
// defined mac address here, needs further configs
#define MAC_ADDR 19
#define SCAN_ERR -1
#define RESPONCE_ERR -1
#define SCAN_OK 0
#define UNKNOWN "unknown"
#define INQ_DURATION 8

/* LE advertising-report scanning (BlueZ), split into open/read/close so a
 * caller can poll() the socket as just another fd rather than block inside
 * a dedicated loop. Linux-only; hci_scan_open() returns SCAN_ERR on any
 * other platform and hci_scan_read()/hci_scan_close() are no-ops there.
 *
 * hci_scan_open()  - configures LE scan + an HCI-event filter and returns
 *                    the socket fd for poll(), or SCAN_ERR on failure.
 * hci_scan_read()  - call once the fd is readable. Consumes exactly one
 *                    HCI event; if it's a Remote ID advert, feeds it to
 *                    the pipeline and sets *updated_idx to the track index
 *                    (left at -1 otherwise). Returns SCAN_OK, or SCAN_ERR
 *                    on a fatal socket read error (caller should stop
 *                    polling this fd and call hci_scan_close()).
 * hci_scan_close() - disables LE scan and closes the socket.
 */
int hci_scan_open(void);
int hci_scan_read(int sock, track_table_t *table, time_t now, const magpie_thresholds_t *th, const magpie_profile_t *profile, int *updated_idx);
void hci_scan_close(int sock);
#endif
