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

result_t magpie_validate_basic_id(const droneID_basicID_data *basic_id){
    result_t result = {
        .rule = KX_RULE_NONE,
        .severity = KX_SEVERITY_INFO,
    };
    if (basic_id == NULL){
        result.rule = KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
        return result;
    }
    switch (basic_id->IDType){
        case DRONE_ID_IDTYPE_NONE:
        case DRONE_ID_IDTYPE_SERIAL_NUMBER:
        case DRONE_ID_IDTYPE_CAA_REGISTRATION_ID:
        case DRONE_ID_IDTYPE_UTM_ASSIGNED_UUID:
        case DRONE_ID_IDTYPE_SPECIFIC_SESSION_ID:
            break;
        default:
            result.rule |= KX_RULE_FIELD_OUT_OF_RANGE;
            result.severity = KX_SEVERITY_ERROR;
            break;
    }
    switch (basic_id->UAType){
        case DRONE_ID_UATYPE_NONE:
        case DRONE_ID_UATYPE_AEROPLANE:
        case DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR:
        case DRONE_ID_UATYPE_GYROPLANE:
        case DRONE_ID_UATYPE_HYBRID_LIFT:
        case DRONE_ID_UATYPE_ORNITHOPTER:
        case DRONE_ID_UATYPE_GLIDER:
        case DRONE_ID_UATYPE_KITE:
        case DRONE_ID_UATYPE_FREE_BALLOON:
        case DRONE_ID_UATYPE_CAPTIVE_BALLOON:
        case DRONE_ID_UATYPE_AIRSHIP:
        case DRONE_ID_UATYPE_FREE_FALL_PARACHUTE:
        case DRONE_ID_UATYPE_ROCKET:
        case DRONE_ID_UATYPE_TETHERED_POWERED_AIRCRAFT:
        case DRONE_ID_UATYPE_GROUND_OBSTACLE:
        case DRONE_ID_UATYPE_OTHER:
            break;
        default:
            result.rule |= KX_RULE_FIELD_OUT_OF_RANGE;
            result.severity = KX_SEVERITY_ERROR;
            break;
    }
    size_t uas_id_len = strnlen(basic_id->UASID, DRONE_ID_SIZE + 1);
    if (uas_id_len == 0){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (uas_id_len > DRONE_ID_SIZE){
        result.rule |= KX_RULE_FIELD_OUT_OF_RANGE;
        result.severity = KX_SEVERITY_ERROR;
    }
    return result;
}

result_t magpie_validate_track(const track_t *track, time_t now){
    result_t result = {
        .rule = KX_RULE_NONE,
        .severity = KX_SEVERITY_INFO,
    };
    if (track == NULL){
        result.rule = KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
        return result;
    }
    if (!track->has_basic_id){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (!track->has_location){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (!track->has_operator_id){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (!track->has_self_id){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (!track->has_system){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    if (!track->has_auth){
        result.rule |= KX_RULE_MISSING_REQUIRED_MSG;
        result.severity = KX_SEVERITY_ERROR;
    }
    return result;
}

uint32_t magpie_validate_jump(const track_t *track){
    return 0;
}
