#include <stddef.h>
#include <stdio.h>
#include "korax/magpie_validator.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
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
        case KX_RULE_COUNTER_REPLAY:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_TRACK;
            *reg=KX_PLAUSABILITY;
            *detail=KX_COUNTER_REPLAY_DETECTED;
            break;
        case KX_RULE_IMPLAUSIBLE_KINEMATIC_TELEPORT:
            *sev=KX_SEVERITY_WARN;  *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_KINEMATIC_TELEPORT_DETECTED;
            break;
        case KX_RULE_MAX_CLIMB_MS:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail=KX_CLIMB_EXCEEDS;
            break;
        case KX_RULE_DUPLICATION_DETECTED:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail=KX_DUPLICATION_DETECTED;
            break;
        case KX_RULE_ID_CHURN:
            *sev=KX_SEVERITY_WARN;
            *field=KX_FIELD_UAS_ID;
            *reg=KX_PLAUSABILITY;
            *detail=KX_ID_CHURN_DETECTED;
            break;
        case KX_RULE_RSSI_POSITION:
            *sev=KX_SEVERITY_WARN;
            *field=KX_FIELD_LOCATION;
            *reg=KX_PLAUSABILITY;
            *detail=KX_RSSI_POS;
            break;
        case KX_RULE_UASID_FORMAT:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail= KX_UASID_FORMAT;
            break;
        case KX_RULE_OPERATOR_ID_FORMAT:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail=KX_OPERATOR_ID_FORMAT;
            break;
        case KX_RULE_EU_CLASS_CONSISTENCY:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail=KX_EU_CLASS;
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
        KX_RULE_COUNTER_REPLAY,
        KX_RULE_OPERATOR_DISTANCE,
        KX_RULE_IMPLAUSIBLE_KINEMATIC_TELEPORT,
        KX_RULE_MAX_CLIMB_MS,
        KX_RULE_DUPLICATION_DETECTED,
        KX_RULE_ID_CHURN,
        KX_RULE_RSSI_POSITION,
        KX_RULE_UASID_FORMAT,
        KX_RULE_OPERATOR_ID_FORMAT,
        KX_RULE_EU_CLASS_CONSISTENCY
};

kx_result_t magpie_validate_location(const DRONE_ID_Location_data *location){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };
    
    if (location == NULL) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
        return r;
    }

    int out_of_range = 0;
    if (location->Latitude < MIN_LAT|| location->Latitude > MAX_LAT) out_of_range = 1;
    if (location->Longitude < MIN_LON || location->Longitude > MAX_LON) out_of_range = 1;
    if (location->AltitudeGeo < MIN_ALT|| location->AltitudeGeo > MAX_ALT) out_of_range = 1;
    if (location->AltitudeBaro < MIN_ALT_BARO || location->AltitudeBaro > MAX_ALT_BARO)out_of_range = 1;
    if (location->TimeStamp != -1.0f && (location->TimeStamp < 0.0f || location->TimeStamp > MAX_TIMESTAMP))out_of_range = 1;
    if (location->Status > DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE) out_of_range = 1;
    
    if (out_of_range) {
        r.rule    |= KX_RULE_FIELD_OUT_OF_RANGE;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

kx_result_t magpie_validate_basic_id(const droneID_basicID_data *basic_id){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };

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
    while (len < DRONE_ID_SIZE && basic_id->UASID[len] != '\0') len++;
    if (len == 0) {
        r.rule    |= KX_RULE_MISSING_REQUIRED_MSG;
        r.severity = KX_SEVERITY_ERROR;
    }
    return r;
}

kx_result_t magpie_validate_track(const track_t *track, time_t now, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };

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
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };
    if (track == NULL || track->msg_count_location < 2 || th == NULL) return r;
    
    double distance_km  = jump_detection(track);
    double elapsed_secs = (double)(track->last_location_seen - track->prev_location_seen);
    if (elapsed_secs <= 0.0) return r;
    
    double speed_kmh = distance_km * 3600.0 / elapsed_secs;
    if (speed_kmh > th->max_speed_kmh) {
        r.rule    |= KX_RULE_IMPLAUSIBLE_JUMP;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_reserved_bitsets(const track_t *track){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };
    if (track == NULL) return r;
    if (track->reserved_bits_seen){
        r.rule    |= KX_RULE_RESERVED_BITS_SET;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_timestamp(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    if (track == NULL || th == NULL) return r;
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

kx_result_t magpie_validate_broadcast_rate(const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
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

kx_result_t magpie_validate_speed_vs_type(const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };

    if (track == NULL || th == NULL) return r;
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
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    
    if (track == NULL || th == NULL) return r;
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

kx_result_t magpie_validate_altitude_envelope(const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    if (track == NULL || th == NULL) return r;
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

kx_result_t magpie_validate_operator_distance(const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    
    if (track == NULL || th == NULL) return r;
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
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_INFO 
    };
    
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

kx_result_t magpie_validate_id_churn(const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_WARN 
    };

    if (track == NULL || th == NULL) return r;
    if (track->id_change_count >= th->id_churn_limit) r.rule = KX_RULE_ID_CHURN;
    return r;
}

kx_result_t magpie_validate_counter_replay(const track_t *track, const uint8_t *new_counter){
      kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_WARN
    };
    if (track == NULL || new_counter == NULL || !track->have_counter) return r;

    uint8_t delta = (uint8_t)(*new_counter - track->last_msg_counter);
    if (delta == 0 || delta > REPLAY_BACKWARD_WINDOW) r.rule = KX_RULE_COUNTER_REPLAY;
    return r;
}

kx_result_t magpie_validate_rssi_position(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    if (table == NULL || track == NULL) return r;

    int flag = 0;
    if (track == NULL || th == NULL) return r;
    if (!th->has_sensor_pos) return r;
    if (!track->has_location) return r;
    if (track->location.Latitude == 0.0 && track->location.Longitude == 0.0) return r;
    if (!track->has_rssi) return r;
    double claimed_dist_km = haversine_km(track->location.Latitude,
                                              track->location.Longitude,
                                              th->sensor_lat,
                                              th->sensor_lon) * 1000.0;
    if (claimed_dist_km < 1.0) claimed_dist_km = 1.0;
    double expected_rssi = KX_RSSI_AT_1M - 10.0 * KX_PATHLOSS_N * log10(claimed_dist_km);
    if ((double)track->last_rssi_dbm > expected_rssi + KX_RSSI_MARGIN_DB) flag = 1;
    if (flag == 1){
        r.rule = KX_RULE_RSSI_POSITION;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

kx_result_t magpie_validate_kinematic_teleport(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    
    if (table == NULL || track == NULL || th == NULL || track->msg_count_location < 2) return r;
    int flag = 0;
    if (!track->has_location) return r;
    if (track->location.Latitude == 0.0 && track->location.Longitude == 0.0) return r;

    double distance_km = jump_detection(track);
    double elapsed_secs = (double)(track->last_location_seen - track->prev_location_seen);
    if (elapsed_secs <= 0.0) return r;

    if ((distance_km * 3600.0 / elapsed_secs) > th->teleport_threshold) flag = 1;

    if (flag == 1){
        r.rule = KX_RULE_IMPLAUSIBLE_KINEMATIC_TELEPORT;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

static bool is_valid_cta2063a(const char *id){
    size_t len = strlen(id);

    if (len < 6 || len > 20) return false;
    for (int i = 0; i < 4; i++){
        char c = id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'))) return false;
    }

    char lc = id[4];
    int n;
    if (lc >= '1' && lc <= '9') n = lc - '0';
    else if (lc >= 'A' && lc <= 'F') n = 10 + (lc - 'A');
    else return false;

    if (len - 5 != (size_t)n) return false;
    for (size_t i = 5; i < len; i++){
        char c = id[i];
        bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z' && c != 'O' && c != 'I');
        if (!ok) return false;
    }
    return true;
}

static bool is_valid_caa_registration(const char *id){
    size_t len = strlen(id);
    if (len == 0 || len > 20) return false;
    for (size_t i = 0; i < len; i++){
        unsigned char c = id[i];
        if (!(isalnum(c) || c == '-' || c == '.')) return false;
    }
    return true;
}

static bool is_valid_utm_assigned_uuid(const char *id){
    uint8_t version = ((uint8_t)id[6] >> 4) & 0x0F;
    if (version < 1 || version > 5) return false;
    
    uint8_t variant = ((uint8_t)id[8] & 0xC0);
    if (variant != 0x80) return false;
    return true;
}

kx_result_t magpie_validate_uasid_format(const droneID_basicID_data *data){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    if (data == NULL) return r;
    
    bool ok = true;
    switch(data->IDType){
        case DRONE_ID_IDTYPE_SERIAL_NUMBER:
            ok = is_valid_cta2063a(data->UASID);
            break;
        case DRONE_ID_IDTYPE_CAA_REGISTRATION_ID:
            ok = is_valid_caa_registration(data->UASID);
            break;
        case DRONE_ID_IDTYPE_UTM_ASSIGNED_UUID:
            ok = is_valid_utm_assigned_uuid(data->UASID);
            break;
        case DRONE_ID_IDTYPE_SPECIFIC_SESSION_ID:
            ok = is_valid_caa_registration(data->UASID);
            break;
        default:
            return r;
    }
    if (!ok){
        r.rule = KX_RULE_UASID_FORMAT;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

static bool is_valid_operator_id(const char *id){
    size_t len = strlen(id);
    if (len < 4 || len > 20) return false;

    for (int i = 0; i < 3; i++){
        if (!(id[i] >= 'A' && id[i] <= 'Z')) return false;
    }
    for (size_t i = 3; i < len; i++){
        unsigned char c = (unsigned char)id[i];
        if (!(isalnum(c) || c == '-')) {
            return false;
        }
    }
    return true;
}

kx_result_t magpie_validate_operator_id_format(const OperatorID_data *data){
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (data == NULL) return r;
    if (!is_valid_operator_id(data->OperatorId)){
        r.rule = KX_RULE_OPERATOR_ID_FORMAT;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

static bool is_valid_eu_class(category_EU_t cat, class_EU_t cls){
    if (cat == CATEGORY_EU_UNDECLARED || cls == CLASS_EU_UNDECLARED) return true;
    switch (cat){
        case CATEGORY_EU_OPEN:
            return (cls >= CLASS_EU_CLASS_0 && cls <= CLASS_EU_CLASS_4);
        case CATEGORY_EU_SPECIFIC:
            return true;
        case CATEGORY_EU_CERTIFIED:
            return false;
        default:
            return true;
    }
}

kx_result_t magpie_validate_eu_class_consistency(const sys_data *data){
    kx_result_t r = { 
        .rule = KX_RULE_NONE, 
        .severity = KX_SEVERITY_ERROR 
    };
    if (data == NULL) return r;
    if (!is_valid_eu_class(data->CategoryEU, data->ClassEU)){
        r.rule = KX_RULE_EU_CLASS_CONSISTENCY;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter){
    uint32_t flags = KX_RULE_NONE;
    if (track == NULL || th == NULL){
        flags |= KX_VALIDATION_FAILURE;
        return flags;
    }

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
    flags |= magpie_validate_id_churn(track, th).rule;
    flags |= magpie_validate_counter_replay(track, msg_counter).rule;
    flags |= magpie_validate_rssi_position(table, track, th).rule;
    flags |= magpie_validate_kinematic_teleport(table, track, th).rule;
    if (msg != NULL) {
        switch (msg->type) {
            case MESSAGETYPE_BASIC_ID:
                flags |= magpie_validate_basic_id(&msg->data.basic_id).rule;
                flags |= magpie_validate_uasid_format(&msg->data.basic_id).rule;
                break;
            case MESSAGETYPE_LOCATION:
                flags |= magpie_validate_location(&msg->data.location).rule;
                break;
            case MESSAGETYPE_OPERATOR_ID:
                flags |= magpie_validate_operator_id_format(&msg->data.operator_id).rule;
                break;
            case MESSAGETYPE_SYSTEM:
                flags |= magpie_validate_eu_class_consistency(&msg->data.system).rule;
                break;
            default:
                break;
        }
    }
    return flags;
}

kx_verdict_t magpie_evaluate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter, const magpie_profile_t *profile){
    kx_verdict_t verdict = {
        .flags = KX_RULE_NONE,
        .compliant = true,
        .ruleset_version = KX_RULESET_VERSION,
        .finding_count = 0
    };
    if (track == NULL || th == NULL || profile == NULL) return verdict;

    verdict.flags = magpie_validate(track, msg, now, th, table, msg_counter) & ~profile->disabled_rules;

    size_t i;
    for (i = 0; i < sizeof(all) / sizeof(all[0]) && verdict.finding_count < KX_MAX_FINDINGS; i++){
        if (verdict.flags & all[i]){
            kx_finding_t *f = &verdict.findings[verdict.finding_count++];
            f->rule = all[i];
            rule_meta(all[i], &f->severity, &f->field, &f->reg_ref, &f->detail);
        }
    }
    verdict.compliant = (verdict.flags & KX_RULE_ERROR_MASK) == 0;
    return verdict;
}

