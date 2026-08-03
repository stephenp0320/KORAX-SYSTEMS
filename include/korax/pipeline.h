#ifndef PIPELINE_H
#define PIPELINE_H
#include "korax/raven_decode.h"
#include "korax/capture.h"
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"
#define PIPELINE_ERR -1

int process_rid_message(const uint8_t *rid_msg, int len, track_table_t *table);

int process_decoded_message(track_table_t *table, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const ble_adv_frame_t *frame, const uint8_t *msg_counter);
int process_ble_frame(const ble_adv_frame_t *ble_frame, time_t now, track_table_t *table);

int process_message_pack(const uint8_t *packet, int len, track_table_t *table);

#endif
