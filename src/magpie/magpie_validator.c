#include <stddef.h>
#include "korax/magpie_validator.h"

kx_result_t magpie_validate_location(const DRONE_ID_Location_data *location){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };
    if (location == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    int out_of_range = 0;
    if (location->Latitude     < MIN_LAT      || location->Latitude     > MAX_LAT)      out_of_range = 1;
    if (location->Longitude    < MIN_LON      || location->Longitude    > MAX_LON)      out_of_range = 1;
    if (location->AltitudeGeo  < MIN_ALT      || location->AltitudeGeo  > MAX_ALT)      out_of_range = 1;
    if (location->AltitudeBaro < MIN_ALT_BARO || location->AltitudeBaro > MAX_ALT_BARO) out_of_range = 1;
    if (location->TimeStamp != -1.0f &&
        (location->TimeStamp < 0.0f || location->TimeStamp > MAX_TIMESTAMP))            out_of_range = 1;
    if (location->Status > DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE)                    out_of_range = 1;

    if (out_of_range) {
        r.rule    |= KX_RULE_FIELD_OUT_OF_RANGE;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

kx_result_t magpie_validate_basic_id(const droneID_basicID_data *basic_id){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };

    if (basic_id == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    if (basic_id->IDType > DRONE_ID_IDTYPE_SPECIFIC_SESSION_ID) {
        r.rule    |= KX_RULE_FIELD_OUT_OF_RANGE;
        r.severity = KX_SEVERITY_ERROR;
    }

    size_t len = 0;
    while (len < DRONE_ID_SIZE && basic_id->UASID[len] != '\0')
        len++;
    if (len == 0) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

kx_result_t magpie_validate_track(const track_t *track, time_t now){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };

    if (track == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    if (!track->has_basic_id || !track->has_location ||
        !track->has_system   || !track->has_operator_id) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
    }

    if (track->has_location &&
        now - track->last_location_seen > KX_LOCATION_GRACE_SECONDS) {
        r.rule |= KX_RULE_NO_LOCATION_IN_GRACE;
        if (KX_SEVERITY_ERROR > r.severity)
            r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

kx_result_t magpie_validate_jump(const track_t *track){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };

    if (track == NULL || track->msg_count_location < 2)
        return r;

    double distance_km  = jump_detection(track);
    double elapsed_secs = (double)(track->last_location_seen - track->prev_location_seen);
    if (elapsed_secs <= 0.0)
        return r;

    double speed_kmh = distance_km * 3600.0 / elapsed_secs;
    if (speed_kmh > MAX_SPEED_KMH) {
        r.rule    |= KX_RULE_IMPLAUSIBLE_JUMP;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now){
    uint32_t flags = KX_RULE_NONE;
    flags |= magpie_validate_track(track, now).rule;
    flags |= magpie_validate_jump(track).rule;
    if (msg != NULL) {
        switch (msg->type) {
            case MESSAGETYPE_BASIC_ID:
                flags |= magpie_validate_basic_id(&msg->data.basic_id).rule;
                break;
            case MESSAGETYPE_LOCATION:
                flags |= magpie_validate_location(&msg->data.location).rule;
                break;
            default:
                break;
        }
    }
    return flags;
}
