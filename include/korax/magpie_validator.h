/*
 * Korax — Magpie validation engine (public API, work in progress).
 *
 * Magpie decides whether a decoded drone broadcast can be trusted. It is a
 * pure function of a track plus the arriving message: it returns a set of
 * flags with stable IDs, so downstream consumers and compliance reports can
 * reference them. See the KAN-4 (Magpie) epic for the rule catalogue.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KORAX_MAGPIE_VALIDATOR_H
#define KORAX_MAGPIE_VALIDATOR_H
#define MAX_SPEED_KMH 150.0
#include "korax/raven_decode.h"
#include "korax/rook_track.h"

/* Rule severities. A track is compliant when no ERROR-severity flag is set. */
typedef enum {
    KX_SEVERITY_INFO = 0,
    KX_SEVERITY_WARN,
    KX_SEVERITY_ERROR,
} kx_severity_t;

/* Stable validator rule identifiers (v1 catalogue — see KAN-4). */
typedef enum {
    KX_RULE_NONE = 0,
    KX_RULE_MISSING_REQUIRED_MSG = 1u << 0,   /* required message type absent          */
    KX_RULE_NO_LOCATION_IN_GRACE = 1u << 1,/* no Location within the grace window   */
    KX_RULE_RESERVED_BITS_SET = 1u << 2,      /* reserved fields set                   */
    KX_RULE_FIELD_OUT_OF_RANGE = 1u << 3,     /* value outside its spec-defined range  */
    KX_RULE_IMPLAUSIBLE_JUMP = 1u << 4,       /* position/speed jump (uses prev fix)   */
} kx_rule_t;

/*
 * Evaluate a track against the v1 rule set. Pure function: no allocation,
 * no I/O. Returns a bitmask of tripped rules (0 == compliant).
 *
 * TODO: implement (KAN-4). Declared here so the public API is stable.
 */

uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now);

#endif /* KORAX_MAGPIE_VALIDATOR_H */
