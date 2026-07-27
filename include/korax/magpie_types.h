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
#endif
