#include <stddef.h>
#include <stdio.h>
#include "korax/magpie_validator.h"
#include <math.h>
#include <string.h>
#define EARTH_RADIUS_KM 6371.0

static void rule_meta(kx_rule_t rule, kx_severity_t *sev, const char **field, const char **reg, const char **detail){
    switch (rule){
        case KX_RULE_MISSING_REQUIRED_MSG:
            *sev=KX_SEVERITY_ERROR; *field=KX_FIELD_TRACK;
            *reg=KX_REG_EU_945;
            *detail=KX_REQ_MSG_TYPE_MISSING;
            break;
        case KX_RULE_NO_LOCATION_IN_GRACE:
            *sev=KX_SEVERITY_ERROR; *field=KX_FIELD_LOCATION;
            *reg=KX_REG_EN_4709;
            *detail=KX_NO_LOC_UPDATE;
            break;
        case KX_RULE_FIELD_OUT_OF_RANGE:
            *sev=KX_SEVERITY_ERROR; *field=KX_FIELD_LOCATION;
            *reg=KX_REG_F3411;
            *detail=KX_FIELD_VAL_OUTSIDE_SPEC_RANGE;
            break;
        case KX_RULE_RESERVED_BITS_SET:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_RESERVED;
            *reg=KX_REG_F3411;
            *detail=KX_RES_BITS_OBSERVED_SET;
            break;
        case KX_RULE_IMPLAUSIBLE_JUMP:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_POS_JUMP_EXCEEDS_MAX_SPEED;
            break;
        case KX_RULE_TIMESTAMP_REWIND:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_LOC_TIMESTAMP_MOVED_BACKWARDS;
            break;
        case KX_RULE_BROADCAST_RATE:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_TRACK;
            *reg=KX_REG_EN_4709;
            *detail=KX_BROADCAST_RATE_BELOW_REQ_MIN;
            break;
        case KX_RULE_SPEED_EXCEEDS_TYPE:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_SPEED_EXCEEDS_ENVELOPE;
            break;
        case KX_RULE_ALT_ENVELOPE:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_ALT_OUTSIDE_PLAUSIBLE_ENVELOPE;
            break;
        case KX_RULE_OPERATOR_DISTANCE:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_DRONE_OPERATOR_LOC_TOO_FAR;
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
        KX_RULE_TIMESTAMP_REWIND,
        KX_RULE_BROADCAST_RATE,
        KX_RULE_SPEED_EXCEEDS_TYPE,
        KX_RULE_ALT_ENVELOPE,
        KX_RULE_OPERATOR_DISTANCE,
};

kx_result_t magpie_validate_location(const DRONE_ID_Location_data *location){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };
    if (location == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    int out_of_range = 0;
    if (location->Latitude < MIN_LAT|| location->Latitude > MAX_LAT){
        out_of_range = 1;
    }
    if (location->Longitude < MIN_LON || location->Longitude > MAX_LON){
        out_of_range = 1;
    }
    if (location->AltitudeGeo < MIN_ALT|| location->AltitudeGeo > MAX_ALT){
        out_of_range = 1;
    }
    if (location->AltitudeBaro < MIN_ALT_BARO || location->AltitudeBaro > MAX_ALT_BARO){
        out_of_range = 1;
    }
    if (location->TimeStamp != -1.0f && (location->TimeStamp < 0.0f || location->TimeStamp > MAX_TIMESTAMP)){
        out_of_range = 1;
    }
    if (location->Status > DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE){
        out_of_range = 1;
    }
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

kx_result_t magpie_validate_timestamp(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL) return r;
    (void)th;

    if (track->msg_count_location < 2) return r;
    float now_ts  = track->location.TimeStamp;
    float prev_ts = track->prev_location.TimeStamp;

    if (now_ts < 0.0f || prev_ts < 0.0f) return r;
    if (now_ts < prev_ts){
        r.rule     = KX_RULE_TIMESTAMP_REWIND;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_broadcast_rate(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL || th == NULL) return r;

    uint64_t oldest = 0, newest = 0;
    int count = 0;
    for (int i = 0; i < KX_RATE_WINDOW; i++){
        uint64_t t = track->arrival_ms[i];
        if (t == 0) continue;
        if (count == 0 || t < oldest) oldest = t;
        if (count == 0 || t > newest) newest = t;
        count++;
    }

    if (count < 2) return r;
    double span_s = (double)(newest - oldest) / 1000.0;
    if (span_s <= 0.0) return r;

    double rate_hz = (double)(count - 1) / span_s;
    if (rate_hz < th->min_broadcast_hz){
        r.rule = KX_RULE_BROADCAST_RATE;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

static double max_speed_ms_for_type(DRONE_ID_uatype_t t){
    switch (t){
        case DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR:
            return DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR_MAX_SPEED;
        case DRONE_ID_UATYPE_AEROPLANE:
            return DRONE_ID_UATYPE_AEROPLANE_MAX_SPEED;
        case DRONE_ID_UATYPE_GYROPLANE:
            return DRONE_ID_UATYPE_GYROPLANE_MAX_SPEED;
        case DRONE_ID_UATYPE_HYBRID_LIFT:
            return DRONE_ID_UATYPE_HYBRID_LIFT_MAX_SPEED;
        case DRONE_ID_UATYPE_GLIDER:
            return DRONE_ID_UATYPE_GLIDER_MAX_SPEED;
        default:
            return UNDEFINED_MAX_SPEED;
    }
}

kx_result_t magpie_validate_speed_vs_type(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL) return r;
    (void)th;

    if (!track->has_basic_id || !track->has_location) return r;

    float speed = track->location.SpeedHorizontal;
    if (speed >= (float)INV_SPEED_H) return r;
    double cap = max_speed_ms_for_type(track->basic_id.UAType);
    if (cap < 0.0) return r;

    if ((double)speed > cap){
        r.rule = KX_RULE_SPEED_EXCEEDS_TYPE;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_climb_rate(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL) return r;
    (void)th;
    if (!track->has_location) return r;

    float ver_speed = track->location.SpeedVertical;
    if (ver_speed >= (float)INV_SPEED_V) return r;
    float magnitude = (ver_speed < 0.0f) ? -ver_speed : ver_speed;
    if (magnitude > th->max_climb_ms){
        r.rule     = KX_RULE_MAX_CLIMB_MS;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_altitude_envelope(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL) return r;
    (void)th;

    if (!track->has_location) return r;
    float alt = track->location.AltitudeGeo;
    if (alt <= (float)INV_ALT) return r;
    if (alt > th->max_altitude_m || alt < th->altitude_floor_m){
        r.rule = KX_RULE_ALT_ENVELOPE;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

double haversine_km(double lat1, double lon1, double lat2, double lon2){
    double phi1        = to_radians(lat1);
    double phi2        = to_radians(lat2);
    double delta_phi   = to_radians(lat2 - lat1);
    double delta_lambda= to_radians(lon2 - lon1);
    double a = sin(delta_phi/2.0) * sin(delta_phi/2.0) + cos(phi1) * cos(phi2) * sin(delta_lambda/2.0) * sin(delta_lambda/2.0);

    double c = 2.0 * asin(sqrt(a));
    return EARTH_RADIUS_KM * c;
}

kx_result_t magpie_validate_operator_distance(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL) return r;
    (void)th;
    if (!track->has_location || !track->has_system) return r;

    double dlat = track->location.Latitude, dlon = track->location.Longitude;
    double olat = track->system.OperatorLatitude, olon = track->system.OperatorLongitude;

    if ((dlat == 0.0 && dlon == 0.0) || (olat == 0.0 && olon == 0.0)) return r;
    double dist = haversine_km(dlat, dlon, olat, olon);
    if (dist > th->max_operator_distance_km){
        r.rule = KX_RULE_OPERATOR_DISTANCE;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_id_clone(const track_t *track, const track_table_t *table){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_INFO };
    if (track == NULL || table == NULL) return r;
    if (track->uas_id[0] == '\0') return r;

    int count = 0;
    for (int i = 0; i < table->num_tracks; i++){
        if (strcmp(table->tracks[i].uas_id, track->uas_id) == 0){
            count++;
        }
    }
    if (count > 1){
        r.rule     = KX_RULE_DUPLICATION_DETECTED;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}
kx_result_t magpie_validate_id_churn(const track_t *track, const track_table_t *table)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL || table == NULL) return r;
    if (track->source_mac[0] == '\0') return r;

    int count = 0;
    for (int i = 0; i < table->num_tracks; i++){
        if (memcmp(table->tracks[i].source_mac, track->source_mac, sizeof track->source_mac) == 0){
            count++;
        }
    }
    if (count > 1){
        r.rule = KX_RULE_ID_CHURN;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_counter_replay(const track_table_t *table, const track_t *track)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (table == NULL || track == NULL) return r;

    uint8_t new_msg_counter = 0;
    uint8_t last_msg_counter = 0;
    uint8_t flag = 0;
    for (int i = 0; i < table->num_tracks; i++){
        new_msg_counter = track->head;
        last_msg_counter = table->tracks[i].last_msg_counter;
        uint8_t delta = (new_msg_counter - last_msg_counter);
        if (delta == 0 || delta > REPLAY_BACKWARD_WINDOW){
            flag = 1;
        }
    }
    if (flag == 1){
        r.rule = KX_RULE_COUNTER_REPLAY;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table)
{
    uint32_t flags = KX_RULE_NONE;
    flags |= magpie_validate_track(track, now, th).rule;
    flags |= magpie_validate_jump(track, th).rule;
    flags |= magpie_validate_reserved_bitsets(track).rule;
    flags |= magpie_validate_timestamp(track, th).rule;
    flags |= magpie_validate_speed_vs_type(track, th).rule;
    flags |= magpie_validate_climb_rate(track, th).rule;
    flags |= magpie_validate_altitude_envelope(track, th).rule;
    flags |= magpie_validate_operator_distance(track, th).rule;
    flags |= magpie_validate_broadcast_rate(track, th).rule;
    flags |= magpie_validate_id_clone(track, table).rule;
    flags |= magpie_validate_id_churn(track, table).rule;

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



kx_verdict_t magpie_evaluate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table){
    kx_verdict_t verdict = {
        .flags = KX_RULE_NONE,
        .compliant = true,
        .ruleset_version = KX_RULESET_VERSION,
        .finding_count = 0
    };
    if (track == NULL) return verdict;

    verdict.flags = magpie_validate(track, msg, now, th, table);
    verdict.flags |= magpie_validate_reserved_bitsets(track).rule;
    verdict.flags |= magpie_validate_timestamp(track, th).rule;
    verdict.flags |= magpie_validate_speed_vs_type(track, th).rule;
    verdict.flags |= magpie_validate_climb_rate(track, th).rule;
    verdict.flags |= magpie_validate_altitude_envelope(track, th).rule;
    verdict.flags |= magpie_validate_operator_distance(track, th).rule;
    verdict.flags |= magpie_validate_broadcast_rate(track, th).rule;
    verdict.flags |= magpie_validate_id_clone(track, table).rule;
    verdict.flags |= magpie_validate_id_churn(track, table).rule;

    size_t i;
    for (i = 0; i < sizeof(all) / sizeof(all[0]); i++){
        if (verdict.flags & all[i]){
            kx_finding_t *f = &verdict.findings[verdict.finding_count++];
            f->rule = all[i];
            rule_meta(all[i], &f->severity, &f->field, &f->reg_ref, &f->detail);
        }
    }
    verdict.compliant = (verdict.flags & KX_RULE_ERROR_MASK) == 0;
    return verdict;
}

