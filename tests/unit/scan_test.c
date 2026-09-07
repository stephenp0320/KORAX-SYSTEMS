#include <stdio.h>
#include <time.h>
#include <korax/scanner.h>
#include <korax/rook_track.h>
#include <korax/magpie_validator.h>

int main(void)
{
    track_table_t table;
    track_table_init(&table);

    int sock = hci_scan_open();
    if (sock < 0){
        fprintf(stderr, "hci_scan_open failed\n");
        return 1;
    }

    printf("starting scan (Ctrl+C to stop)...\n");
    int idx;
    while (hci_scan_read(sock, &table, time(NULL), &MAGPIE_DEFAULT_THRESHOLDS, &MAGPIE_DEFAULT_PROFILE, &idx) == SCAN_OK){
        if (idx >= 0){
            printf("track updated: %s\n", table.tracks[idx].uas_id);
        }
    }

    fprintf(stderr, "hci_scan_read failed\n");
    hci_scan_close(sock);
    return 1;
}
