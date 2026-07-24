#include <stddef.h>
#include "korax/magpie_validator.h"
#include "korax/output_json.h"

static void rule_meta(kx_rule_t rule, kx_severity_t *sev, const char **field, const char **reg, const char **detail){
    switch (rule){
        case KX_RULE_MISSING_REQUIRED_MSG:
            *sev=KX_SEVERITY_ERROR; *field="track";
            *reg="EU 2019/945";
            *detail="a required message type is missing";
            break;
        case KX_RULE_NO_LOCATION_IN_GRACE:
            *sev=KX_SEVERITY_ERROR; *field="location";
            *reg="EN 4709-002 DRI";
            *detail="no location update within the grace window";
            break;
        case KX_RULE_FIELD_OUT_OF_RANGE:
            *sev=KX_SEVERITY_ERROR; *field="location";
            *reg="ASTM F3411";
            *detail="a field value is outside its spec range";
            break;
        case KX_RULE_RESERVED_BITS_SET:
            *sev=KX_SEVERITY_WARN;  *field="reserved";
            *reg="ASTM F3411";
            *detail="reserved bits observed set";
            break;
        case KX_RULE_IMPLAUSIBLE_JUMP:
            *sev=KX_SEVERITY_WARN;  *field="location";
            *reg="plausibility";
            *detail="position jump exceeds max speed";
            break;
        default:
            *sev=KX_SEVERITY_INFO;
            *field="";
            *reg="";
            *detail="";
            break;
    }
}

static const kx_rule_t all[] = {
        KX_RULE_MISSING_REQUIRED_MSG,
        KX_RULE_NO_LOCATION_IN_GRACE,
        KX_RULE_RESERVED_BITS_SET,
        KX_RULE_FIELD_OUT_OF_RANGE,
        KX_RULE_IMPLAUSIBLE_JUMP,
};

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

kx_result_t magpie_validate_track(const track_t *track, time_t now, const magpie_thresholds_t *th){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };

    if (track == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    if (!track->has_basic_id || !track->has_location || !track->has_system   || !track->has_operator_id) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
    }

    if (track->has_location && now - track->last_location_seen > th->grace_seconds) {
        r.rule |= KX_RULE_NO_LOCATION_IN_GRACE;
        if (KX_SEVERITY_ERROR > r.severity){
            r.severity = KX_SEVERITY_ERROR;
        }
    }
    return r;
}

kx_result_t magpie_validate_jump(const track_t *track,  const magpie_thresholds_t *th){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };

    if (track == NULL || track->msg_count_location < 2){
        return r;
    }

    double distance_km  = jump_detection(track);
    double elapsed_secs = (double)(track->last_location_seen - track->prev_location_seen);
    if (elapsed_secs <= 0.0){
        return r;
    }
    double speed_kmh = distance_km * 3600.0 / elapsed_secs;
    if (speed_kmh > th->max_speed_kmh) {
        r.rule    |= KX_RULE_IMPLAUSIBLE_JUMP;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_reserved_bitsets(const track_t *track){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };
    if (track == NULL){
        return r;
    }
    if (track->reserved_bits_seen){
        r.rule    |= KX_RULE_RESERVED_BITS_SET;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

// return type of uint32_t is limiting, adding verification structs instead
uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th){
    uint32_t flags = KX_RULE_NONE;
    flags |= magpie_validate_track(track, now, th).rule;
    flags |= magpie_validate_jump(track, th).rule;
    flags |= magpie_validate_reserved_bitsets(track).rule;

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

kx_verdict_t magpie_evaluate(const track_t *track, decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, kx_rule_t rule){
    kx_verdict_t verdict = {
        .flags = KX_RULE_NONE,
        .compliant = true,
        .ruleset_version = KX_RULESET_VERSION,
        .finding_count = 0
    };
    if (track == NULL) return verdict;

    verdict.flags = magpie_validate(track, msg, now, th);
    verdict.flags |= magpie_validate_reserved_bitsets(track).rule;

    size_t i;
    for (i = 0; i < sizeof(all) / sizeof(all[0]); i++){
        // bitwise check using &
        if (verdict.flags & all[i]){
            kx_finding_t *f = &verdict.findings[verdict.finding_count++];
            f->rule = all[i];
            rule_meta(rule, &f->severity, &f->field, &f->reg_ref, &f->detail);
        }
    }
    verdict.compliant = (verdict.flags & KX_RULE_ERROR_MASK) == 0;
    char buffer[KX_MAX_FINDINGS + 1];
    verdict_to_json(&verdict, buffer, sizeof(buffer));
    return verdict;
}

