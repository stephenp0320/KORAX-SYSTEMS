#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/capture.h"
#include "korax/pipeline.h"

static void handle_frame(const uint8_t *buf, int len, track_table_t *table){
    time_t now = time(NULL);
    uint8_t type = (buf[0] >> 4) & 0x0F;
    if (type == MESSAGETYPE_PACKED) {
        decode_message_pack((uint8_t *)buf, len, now, table, &MAGPIE_DEFAULT_THRESHOLDS);
    } else {
        decoded_message_t msg;
        if (decode_message(buf, len, &msg) == DECODE_OK){
            process_decoded_message(table, &msg, now, &MAGPIE_DEFAULT_THRESHOLDS, NULL, NULL, &MAGPIE_DEFAULT_PROFILE);
        }
    }
}

static void replay_frame_callback(const wp_frame_t *frame, void *user_data){
    track_table_t *table = (track_table_t *)user_data;
    handle_frame(frame->payload, (int)frame->payload_len,table);
}

int main(int argc, char **argv){
    const char *filename = (argc > 1) ? argv[1] : "frames.txt";
    track_table_t table;
    track_table_init(&table);
    
    int frames = wp_replay_run(filename, replay_frame_callback, &table);
    if (frames < 0)return EXIT_FAILURE;
    
    printf("Replayed %d frame(s) from %s\n", frames, filename);
    printf("Tracks seen: %d\n\n", table.num_tracks);
    for (int i = 0; i < table.num_tracks; i++){
        track_t *t = &table.tracks[i];
        printf("Drone %d\n", i + 1);
        printf("  UAS ID     : %s\n", t->uas_id);
        printf("  Operator   : %s\n", t->has_operator_id ? t->operator_id.OperatorId : "(none)");
        printf("  Position   : %.5f, %.5f\n", t->location.Latitude, t->location.Longitude);
        printf("  Altitude   : %.1f m (geodetic)\n", t->location.AltitudeGeo);
        printf("  Speed/Head : %.1f m/s @ %.0f deg\n", t->location.SpeedHorizontal, t->location.Direction);
        printf("  Messages   : basic=%d location=%d system=%d operator=%d self=%d\n", t->has_basic_id, t->has_location, t->has_system, t->has_operator_id, t->has_self_id);
        printf("  Compliant  : %s (flags 0x%02X)\n", t->verdict.compliant ? "yes" : "no", t->verdict.flags);
        printf("\n");
    }
    return EXIT_SUCCESS;
}
