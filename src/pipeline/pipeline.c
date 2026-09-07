/*
 * Pipeline.
 *
 * Wires the data-flow spine together: RAVEN decode output -> ROOK track
 * find/create/update -> MAGPIE validation -> verdict stored on the track.
 * This is the only place that decides how an incoming message maps onto a
 * track, so the keying rules below are load-bearing for every caller.
 */
#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include "korax/capture.h"
#include "korax/magpie_validator.h"
#include <stdbool.h>
#include "korax/pipeline.h"
#include "korax/scanner.h"

/*
 * Returns a track index (>= 0) on success, negative (TRACK_ERR /
 * TRACK_NOT_FOUND) on error. Callers must test `< 0` — never `!= 0` — since
 * index 0 is a perfectly valid track.
 */
int process_decoded_message(track_table_t *table, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const ble_adv_frame_t *frame, const uint8_t *msg_counter, const magpie_profile_t *profile){
    if (table == NULL || msg == NULL) return TRACK_ERR;

    int idx;
    /*
     * Keying strategy hinges entirely on whether we were handed a capture
     * frame. `frame != NULL` means live capture: key by the radio-layer
     * source_mac, which is the only reliable identity we have across
     * message types. `frame == NULL` means file replay: fall back to
     * UAS-ID keying (see below). Every replay/pack path must pass NULL.
     */
    if (frame != NULL){
        idx = find_track_by_mac(table, frame->source_mac);
        if (idx == TRACK_NOT_FOUND){
            /* Only a Basic ID message carries the UAS ID a new track needs;
             * anything else for an unseen MAC has no home yet. */
            if (msg->type != MESSAGETYPE_BASIC_ID) return TRACK_NOT_FOUND;
            idx = track_table_create_by_source_mac(table, now, frame->source_mac);
            if (idx < 0) return idx;
        }
    } else {
        if (msg->type == MESSAGETYPE_BASIC_ID){
            idx = find_track(table, msg->data.basic_id.UASID);
            if (idx == TRACK_NOT_FOUND){
                idx = track_table_create(table, now, msg->data.basic_id.UASID);
                if (idx < 0) return idx;
            }
        } else {
            /*
             * Non-Basic-ID message with no MAC to key on (replay path) and
             * no UAS ID in the message itself. Falling back to tracks[0]
             * is only safe here because replay is assumed single-track;
             * this must never be reached on the live/MAC-keyed branch
             * above, which is what test_mac_multi_drone guards against.
             */
            if (table->num_tracks == 0) return TRACK_NOT_FOUND;
            idx = 0;
        }
    }
    track_t *track = &table->tracks[idx];

    int rc = track_table_update(track, now, msg);
    if (rc != TRACK_OK) return rc;

    track->reserved_bits_seen |= msg->reserved_bits;
    /* Evaluate before committing the new counter: Magpie's replay/duplicate
     * checks need to see the track's previous last_msg_counter, not this
     * message's. */
    kx_verdict_t verdict = magpie_evaluate(track, msg, now, th, table, msg_counter, profile);
    if (msg_counter != NULL){
        track->last_msg_counter = *msg_counter;
        track->have_counter = true;
    }
    track->flags = verdict.flags;
    track->is_compliant = verdict.compliant;
    track->finding_count = verdict.finding_count;
    memcpy(track->findings, verdict.findings, sizeof track->findings);
    return idx;
}

/*
 * Live-BLE entry point: extract the ODID payload from the advertisement,
 * decode it, then hand off to process_decoded_message() with the frame
 * present (so it keys by MAC). Same return-index convention applies.
 */
int process_ble_frame(const ble_adv_frame_t *ble_frame, time_t now, track_table_t *table){
    uint8_t rid_msg[RID_MSG_SIZE];
    uint8_t msg_counter = 0;
    decoded_message_t msg;

    if (ble_rid_extraction(ble_frame, rid_msg, &msg_counter) != 0) return -1;
    if (decode_message(rid_msg, RID_MSG_SIZE, &msg) != 0) return -1;

    int idx = process_decoded_message(table, &msg, now, &MAGPIE_DEFAULT_THRESHOLDS, ble_frame, &msg_counter, &MAGPIE_DEFAULT_PROFILE);
    if (idx >= 0){
        table->tracks[idx].last_rssi_dbm = ble_frame->rssi_dbm;
    }
    return idx;
}

