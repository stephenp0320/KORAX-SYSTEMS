#include <stdio.h>
#include "korax/rook_track.h"
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"

typedef struct {
    kx_rule_t rule;
    kx_severity_t severity;
} result_t;

result_t magpie_validate_location(const DRONE_ID_Location_data *location){
    result_t result = {
        .rule = KX_RULE_NONE,
        .severity = KX_SEVERITY_INFO
    };
    if (location == NULL) {
        result.rule = KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
        return result;
    }
    if (location->Latitude < MIN_LAT || location->Latitude > MAX_LAT) {
        result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (location->Longitude < MIN_LON || location->Longitude > MAX_LON) {
        result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (location->AltitudeGeo < MIN_ALT || location->AltitudeGeo > MAX_ALT) {
        result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (location->AltitudeBaro < MIN_ALT_BARO || location->AltitudeBaro > MAX_ALT_BARO) {
        result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (location->TimeStamp < 0.0f || location->TimeStamp > MAX_TIMESTAMP) {
        result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    switch (location->Status) {
        case DRONE_ID_STATUS_UNDECLARED:
        case DRONE_ID_STATUS_GROUND:
        case DRONE_ID_STATUS_AIRBORNE:
        case DRONE_ID_STATUS_EMERGENCY:
        case DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE:
            break;
        default:
            result.rule = KX_RULE_FIELD_OUT_OF_RANGE;
            result.severity = KX_SEVERITY_ERROR;
            break;
    }
    return result;
}

uint32_t magpie_validate_basic_id(const droneID_basicID_data *basic_id){
    return 0;
}

uint32_t magpie_validate_track(const track_t *track, time_t now){
    return 0;
}

uint32_t magpie_validate_jump(const track_t *track){
    return 0;
}
