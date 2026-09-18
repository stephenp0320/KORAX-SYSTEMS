/*
 * Korax — pipeline glue (public API).
 *
 * The pipeline is the spine that ties everything together: RAVEN decode
 * output goes in, ROOK track-table update and MAGPIE validation happen in
 * between, and a track index comes out. Source (live BLE vs file replay) is
 * interchangeable above this layer — both call the same entry points here.
 */
#ifndef PIPELINE_H
#define PIPELINE_H
#include "korax/raven_decode.h"
#include "korax/capture.h"
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"
#define PIPELINE_ERR -1
#define PROCESS_BLE_FRAME_ERR -1
#define PROCESS_BLE_FRAME_OK 0

/* Reserved entry point for an already-extracted RID message (no capture
 * frame); not yet implemented. */
int process_rid_message(const uint8_t *rid_msg, int len, track_table_t *table);

/*
 * Core pipeline step: find/create the track, fold the message into it, run
 * Magpie, and store the verdict. Returns a track index (>= 0) on success,
 * negative (TRACK_ERR / TRACK_NOT_FOUND) on error — callers MUST check
 * `< 0`, never `!= 0`, since index 0 is a valid track.
 *
 * `frame` selects the keying strategy: non-NULL means live capture, so the
 * track is keyed by `frame->source_mac`; NULL means file replay, so the
 * track falls back to UAS-ID keying. All replay/pack call paths must pass
 * NULL here.
 */
int process_decoded_message(track_table_t *table, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const ble_adv_frame_t *frame, const uint8_t *msg_counter, const magpie_profile_t *profile);

/* Live-BLE entry point: extracts + decodes the frame, then calls
 * process_decoded_message() with a non-NULL frame (MAC keying). Same
 * return-index convention as process_decoded_message(). */
int process_ble_frame(const ble_adv_frame_t *ble_frame, time_t now, track_table_t *table, const magpie_thresholds_t *th, const magpie_profile_t *profile);

/* Reserved entry point for a packed/batched replay message; not yet
 * implemented. Once added it must call process_decoded_message() with a
 * NULL frame, per the replay/pack keying rule above. */
int process_message_pack(const uint8_t *packet, int len, track_table_t *table);

#endif
