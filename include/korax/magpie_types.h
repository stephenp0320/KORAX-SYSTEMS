#ifndef KORAX_MAGPIE_TYPES_H
#define KORAX_MAGPIE_TYPES_H
#include <stdint.h>
#include <stdbool.h>

#define KX_MAX_FINDINGS    8
#define KX_DETAIL_MAX 128
#define KX_FIELD_TRACK     "track"
#define KX_FIELD_LOCATION  "location"
#define KX_FIELD_RESERVED  "reserved"
#define KX_PLAUSABILITY    "plausability"
#define KX_FIELD_UAS_ID    "basic_id.uas_id"
#define KX_REG_EU_945      "EU 2019/945"
#define KX_REG_EN_4709     "EN 4709-002 DRI"
#define KX_REG_F3411       "ASTM F3411"
#define KX_REQ_MSG_TYPE_MISSING "a required message type is missing"
#define KX_NO_LOC_UPDATE "no location update within the grace window"
#define KX_FIELD_VAL_OUTSIDE_SPEC_RANGE "a field value is outside its specification range"
#define KX_POS_JUMP_EXCEEDS_MAX_SPEED "position jump exceeds maximum speed"
#define KX_RES_BITS_OBSERVED_SET "reserved bits observed set"
#define KX_LOC_TIMESTAMP_MOVED_BACKWARDS "location timestamp moved backwards"
#define KX_BROADCAST_RATE_BELOW_REQ_MIN "broadcast rate below the required minimum"
#define KX_SPEED_EXCEEDS_ENVELOPE "speed exceeds the envelope for the declared aircraft type"
#define KX_ALT_OUTSIDE_PLAUSIBLE_ENVELOPE "altitude outside the plausible envelope"
#define KX_DRONE_OPERATOR_LOC_TOO_FAR "drone implausibly far from declared operator location"
#define DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR_MAX_SPEED 30.0
#define DRONE_ID_UATYPE_AEROPLANE_MAX_SPEED 120.0
#define DRONE_ID_UATYPE_GYROPLANE_MAX_SPEED 60.0
#define DRONE_ID_UATYPE_HYBRID_LIFT_MAX_SPEED 90.0
#define DRONE_ID_UATYPE_GLIDER_MAX_SPEED 70.0
#define UNDEFINED_MAX_SPEED -1.0
#define KX_RULESET_VERSION 1
#define KX_RATE_WINDOW     16

typedef enum { KX_SEVERITY_INFO=0, KX_SEVERITY_WARN, KX_SEVERITY_ERROR } kx_severity_t;

typedef enum {
    KX_RULE_NONE                 = 0,
    KX_RULE_MISSING_REQUIRED_MSG = 1u<<0,
    KX_RULE_NO_LOCATION_IN_GRACE = 1u<<1,
    KX_RULE_RESERVED_BITS_SET    = 1u<<2,
    KX_RULE_FIELD_OUT_OF_RANGE   = 1u<<3,
    KX_RULE_IMPLAUSIBLE_JUMP     = 1u<<4,
    KX_RULE_TIMESTAMP_REWIND     = 1u<<5,
    KX_RULE_BROADCAST_RATE       = 1u<<6,
    KX_RULE_MAX_CLIMB_MS         = 1u<<7,
    KX_RULE_SPEED_EXCEEDS_TYPE   = 1u<<8,
    KX_RULE_ALT_ENVELOPE         = 1u<<9,
    KX_RULE_OPERATOR_DISTANCE    = 1u<<10,
    KX_RULE_CLIMB_RATE           = 1u<<11,
    KX_RULE_DUPLICATION_DETECTED = 1u<<12,
    KX_RULE_ID_CHURN             = 1u<<13
} kx_rule_t;

#define KX_RULE_ERROR_MASK (KX_RULE_MISSING_REQUIRED_MSG | \
                            KX_RULE_NO_LOCATION_IN_GRACE | \
                            KX_RULE_FIELD_OUT_OF_RANGE)

typedef struct { uint32_t rule; kx_severity_t severity; } kx_result_t;

typedef struct {
    kx_rule_t     rule;
    kx_severity_t severity;
    const char   *field;
    const char   *reg_ref;
    const char   *detail;
} kx_finding_t;

typedef struct {
    uint32_t     flags;
    bool         compliant;
    uint16_t     ruleset_version;
    uint8_t      finding_count;
    kx_finding_t findings[KX_MAX_FINDINGS];
} kx_verdict_t;

typedef struct {
    uint64_t arrival_ms[KX_RATE_WINDOW];
    uint8_t head;
    uint8_t last_msg_counter;
    bool have_counter;
    int8_t last_rssi_dbm;
} magpie_track_state_t;

#endif
