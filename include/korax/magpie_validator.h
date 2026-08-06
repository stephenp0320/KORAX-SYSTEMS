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
#include "korax/magpie_types.h"

/* Tunables (make configurable later — KAN-47). */
#define KX_LOCATION_GRACE_SECONDS 5
#define MAX_SPEED_KMH             150.0
#define KX_MIN_BROADCAST_RATE     0.5
#define KX_MAX_CLIMB_MS           20.0
#define KX_ALT_FLOOR              -500.0
#define KX_MAX_ALT                MAX_ALT
#define KX_MAX_OP_DIST            5.0
#define KX_SENSOR_LAT             0.0
#define KX_SENSOR_LON             0.0
#define KX_SENSOR_POS             false
#define KX_PROFILE_NAME "default"
#define KX_DISABLED_RULES 0
#define KX_ENABLED_RULES 0
#define MAGPIE_DEFAULT_THRESHOLDS ((magpie_thresholds_t){ KX_LOCATION_GRACE_SECONDS, MAX_SPEED_KMH, KX_MIN_BROADCAST_RATE, KX_MAX_CLIMB_MS, KX_ALT_FLOOR, KX_MAX_ALT, KX_MAX_OP_DIST , KX_SENSOR_LAT, KX_SENSOR_LON, KX_SENSOR_POS })

#define MAGPIE_DEFAULT_PROFILE ((magpie_profile_t){ KX_PROFILE_NAME, KX_RULESET_VERSION, KX_ENABLED_RULES, MAGPIE_DEFAULT_THRESHOLDS, KX_DISABLED_RULES })

/* NOTE: KX_RULE_ERROR_MASK, KX_MAX_DETAILS/FIELD_MAX/MAX_FINDINGS/RATE_WINDOW
 * below mirror identical macros in magpie_types.h. Identical redefinition is
 * legal C and harmless, but if either copy ever changes, update both. */
#define KX_RULE_ERROR_MASK (KX_RULE_MISSING_REQUIRED_MSG | KX_RULE_NO_LOCATION_IN_GRACE | KX_RULE_FIELD_OUT_OF_RANGE)
#define KX_MAX_DETAILS 256
#define KX_FIELD_MAX 128
#define KX_MAX_FINDINGS 8
#define KX_RATE_WINDOW 16
/* Rule severities. A track is compliant when no ERROR-severity rule is set. */

/* sensor_lat/sensor_lon/has_sensor_pos gate the two sensor-relative checks
 * (magpie_validate_rssi_position, magpie_validate_kinematic_teleport) — both
 * are no-ops until a fixed sensor location is configured. */
typedef struct magpie_thresholds {
    int    grace_seconds;
    double max_speed_kmh;
    double min_broadcast_hz;
    float max_climb_ms;
    float altitude_floor_m;
    float max_altitude_m;
    double max_operator_distance_km;
    double sensor_lat;
    double sensor_lon;
    bool   has_sensor_pos;
} magpie_thresholds_t;

/* disabled_rules is applied as a mask in magpie_evaluate to suppress rule
 * bits before findings are built (the rule-pack gate). enabled_rules is
 * currently unused by magpie_evaluate/magpie_validate — do not assume it
 * gates anything until it is wired up. */
typedef struct magpie_profile_t {
    const char *name;
    uint16_t version;
    uint32_t enabled_rules;
    magpie_thresholds_t thresholds;
    uint32_t disabled_rules;
} magpie_profile_t;

/* Per-aspect checks (pure). Compliance tier — see magpie_validator.c for
 * per-function domain notes. */
kx_result_t magpie_validate_location(const DRONE_ID_Location_data *location);
kx_result_t magpie_validate_basic_id(const droneID_basicID_data *basic_id);
kx_result_t magpie_validate_track(const track_t *track, time_t now, const magpie_thresholds_t *th);
kx_result_t magpie_validate_jump(const track_t *track, const magpie_thresholds_t *th);

/*
 * Composed entry point. Runs every applicable check against the track and the
 * arriving message and returns a bitmask of tripped rules (0 == compliant).
 * Pure: no allocation, no I/O.
 */

uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg,time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter);
kx_result_t magpie_validate_reserved_bitsets(const track_t *track);
kx_verdict_t magpie_evaluate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter, const magpie_profile_t *profile);
/* Integrity / anti-spoof tier (the moat) — cross-track and cross-message checks. */
kx_result_t magpie_validate_id_clone(const track_t *track, const track_table_t *table);
kx_result_t magpie_validate_id_churn(const track_t *track, const track_table_t *table);
kx_result_t magpie_validate_counter_replay(const track_t *track, const uint8_t *new_counter);
kx_result_t magpie_validate_rssi_position(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th);
kx_result_t magpie_validate_kinematic_teleport(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th);
/* P4 rule pack: ID-format / cross-field consistency checks. */
kx_result_t magpie_validate_uasid_format(const droneID_basicID_data *data);
kx_result_t magpie_validate_operator_id_format(const OperatorID_data *data);
kx_result_t magpie_validate_eu_class_consistency(const sys_data *data);
#endif

