/*
 * Korax — Magpie validation engine (public API).
 *
 * Magpie decides whether a decoded drone broadcast can be trusted. Every check
 * is a pure function of a track (plus the arriving message and the current
 * time): no allocation, no I/O. Results are reported as a bitmask of stable
 * rule IDs, so downstream consumers and compliance reports can reference them.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KORAX_MAGPIE_VALIDATOR_H
#define KORAX_MAGPIE_VALIDATOR_H

#include <stdint.h>
#include <time.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"

/* Tunables (make configurable later — KAN-47). */
#define KX_LOCATION_GRACE_SECONDS 5
#define MAX_SPEED_KMH             150.0
#define KX_RULE_ERROR_MASK (KX_RULE_MISSING_REQUIRED_MSG | KX_RULE_NO_LOCATION_IN_GRACE | KX_RULE_FIELD_OUT_OF_RANGE)

/* Rule severities. A track is compliant when no ERROR-severity rule is set. */
typedef enum {
    KX_SEVERITY_INFO = 0,
    KX_SEVERITY_WARN,
    KX_SEVERITY_ERROR,
} kx_severity_t;

/* Stable validator rule identifiers, as bit flags so they OR into a mask. */
typedef enum {
    KX_RULE_NONE                 = 0,
    KX_RULE_MISSING_REQUIRED_MSG = 1u << 0, /* a required message type is absent      */
    KX_RULE_NO_LOCATION_IN_GRACE = 1u << 1, /* no Location within the grace window     */
    KX_RULE_RESERVED_BITS_SET    = 1u << 2, /* reserved fields set                     */
    KX_RULE_FIELD_OUT_OF_RANGE   = 1u << 3, /* value outside its spec-defined range    */
    KX_RULE_IMPLAUSIBLE_JUMP     = 1u << 4, /* position/speed jump (uses previous fix) */
} kx_rule_t;

/* A single check's outcome: which rules tripped (bitmask) + the worst severity. */
typedef struct {
    uint32_t      rule;
    kx_severity_t severity;
} kx_result_t;

/* Per-aspect checks (pure). */
kx_result_t magpie_validate_location(const DRONE_ID_Location_data *location);
kx_result_t magpie_validate_basic_id(const droneID_basicID_data *basic_id);
kx_result_t magpie_validate_track(const track_t *track, time_t now);
kx_result_t magpie_validate_jump(const track_t *track);

/*
 * Composed entry point. Runs every applicable check against the track and the
 * arriving message and returns a bitmask of tripped rules (0 == compliant).
 * Pure: no allocation, no I/O.
 */
uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now);
kx_result_t magpie_validate_reserved_bitsets(const track_t *track);
#endif /* KORAX_MAGPIE_VALIDATOR_H */

