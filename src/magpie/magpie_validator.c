/*
 * Magpie validation engine.
 *
 * Each magpie_validate_<thing>() below is a pure check: track/message/
 * thresholds in, a single kx_result_t out, no allocation, no I/O.
 *
 * magpie_validate() composes them into a rule-bit mask; magpie_evaluate() wraps that m* ask into a kx_verdict_t with human-readable findings (via rule_meta) and the
 * compliance decision.
 */
#include <stddef.h>
#include <stdio.h>
#include "korax/magpie_validator.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#define EARTH_RADIUS_KM 6371.0

/*
 * Maps each rule bit to the finding metadata reported on the wire (severity,
 * field, regulatory reference, human-readable detail).
 * This switch and the kx_rule_t enum in magpie_types.h must stay in lockstep: a rule w* ith no case here silently falls through to `default` and ships blank field/reg/detai* l strings in its finding (see the step-4 recipe in CLAUDE.md).
 *
 * The severity assigned here -- not the severity a check function returns internally —* is what actually reaches the verdict; only bits in KX_RULE_ERROR_MASK affect is_comp* liant regardless of what's set below.
 */
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
        case KX_RULE_CLIMB_RATE:
            *sev=KX_SEVERITY_WARN;
            *reg=KX_PLAUSABILITY;
            *detail=KX_CLIMB_RATE;
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

/*
 * Every rule bit that can appear in a finding, scanned in magpie_evaluate() to
 * build verdict.findings[]. A bit missing from both this table and
 * magpie_validate()'s composition (e.g. KX_RULE_CLIMB_RATE) is effectively
 * dead: it can never be set, so it never surfaces here either.
 */
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

/*
 * Compliance tier. Guards against a Location message with fields outside the
 * ASTM F3411 spec range (lat/lon, geo/baro altitude, timestamp, status enum).
 * A NULL location (message never received) is itself a compliance failure.
 */
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

/*
 * Compliance tier. Guards against a Basic ID message with an out-of-range
 * IDType enum or an empty UAS ID string either means the operator hasn't
 * declared who/what this aircraft is, which EU 2019/945 requires.
 */
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

/*
 * Compliance tier. Guards against two things: (1) the track never having
 * received one of the four required message types, and (2) a location fix
 * that has gone stale, no update within th->grace_seconds (EN 4709-002 DRI
 * live-tracking requirement). Both are ERROR severity.
 */
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

/*
 * Plausibility tier (WARN). Guards against a position jump between two
 * consecutive location fixes that implies a groundspeed faster than
 * th->max_speed_kmh -- physically impossible for the airframe, so likely a
 * decode glitch or spoofed fix.
 */
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

/*
 * WARN severity (not compliance-breaking, despite sitting in the low enum
 * bits). Guards against reserved/spec bits observed set.
 */
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

/*
 * Plausibility tier (WARN). Guards against the Location message's own
 * TimeStamp field moving backwards between two fixes such as a clock rewind or
 * replayed/reordered frame. -1.0f is "no timestamp" and is skipped, not
 * treated as a rewind.
 */
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

/*
 * Plausibility tier. Guards against a track broadcasting slower than
 * th->min_broadcast_hz (EN 4709-002 DRI minimum rate), measured over the
 * KX_RATE_WINDOW most recent arrival timestamps. Note: rule_meta reports this
 * as WARN even though the check locally tags it ERROR, only bits in
 * KX_RULE_ERROR_MASK affect compliance.
 */
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

/* Per-airframe-type speed ceilings (m/s) used by magpie_validate_speed_vs_type
 * below; UNDEFINED_MAX_SPEED means "no envelope known, don't flag".
 */
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

/*
 * Plausibility tier (WARN). Guards against a declared horizontal speed that
 * exceeds the physical envelope for the aircraft's declared UAType (e.g. a
 * "multirotor" reporting 100 m/s). INV_SPEED_H marks "speed not reported".
 */
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

/*
 * Plausibility tier (WARN). Guards against a vertical speed magnitude beyond
 * th->max_climb_ms -- faster climb/descent than any current sUAS airframe can
 * plausibly sustain. INV_SPEED_V marks "vertical speed not reported".
 */
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

/*
 * Plausibility tier (WARN). Guards against a geodetic altitude outside
 * [th->altitude_floor_m, th->max_altitude_m] -- below ground or above any
 * plausible sUAS operating ceiling. INV_ALT marks "altitude not reported".
 */
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

/* Great-circle distance in km. Must return double shared by jump/teleport detection an * d operator-distance.
 */
double haversine_km(double lat1, double lon1, double lat2, double lon2){
    double phi1        = to_radians(lat1);
    double phi2        = to_radians(lat2);
    double delta_phi   = to_radians(lat2 - lat1);
    double delta_lambda= to_radians(lon2 - lon1);
    double a = sin(delta_phi/2.0) * sin(delta_phi/2.0) + cos(phi1) * cos(phi2) * sin(delta_lambda/2.0) * sin(delta_lambda/2.0);

    double c = 2.0 * asin(sqrt(a));
    return EARTH_RADIUS_KM * c;
}

/*
 * Plausibility tier (WARN). Guards against the drone's declared position
 * being implausibly far (> th->max_operator_distance_km) from its declared
 * operator location — a mismatch suggests spoofed or misconfigured System
 * data. (0,0)/(0,0) on either side means "not set", so it's skipped.
 */
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

/*
 * Integrity / anti-spoof tier (WARN). Guards against the same UAS ID being
 * broadcast by more than one live track at once — a likely clone/spoof
 * (two aircraft can't legitimately share a serial number).
 */
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
/*
 * Integrity / anti-spoof tier (WARN). Guards against a single source MAC
 * appearing on more than one track — identity churn, i.e. one radio cycling
 * through multiple UAS IDs (possible spoof). Zero MAC (unset) is skipped.
 */
kx_result_t magpie_validate_id_churn(const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (track == NULL || th == NULL) return r;

    if (track->id_change_count >= th->id_churn_limit){
        r.rule = KX_RULE_ID_CHURN;
    }

    return r;
}

/*
 * Integrity / anti-spoof tier (WARN). Guards against a replayed or
 * out-of-order message counter: delta of 0 (exact repeat) or a backward jump
 * larger than REPLAY_BACKWARD_WINDOW (treated as wrap-then-rewind, not
 * legitimate forward progress on an 8-bit counter).
 */
kx_result_t magpie_validate_counter_replay(const track_t *track, const uint8_t *new_counter)
{
      kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_WARN };
      if (track == NULL || new_counter == NULL || !track->have_counter) return r;

      uint8_t delta = (uint8_t)(*new_counter - track->last_msg_counter);
      if (delta == 0 || delta > REPLAY_BACKWARD_WINDOW){
          r.rule = KX_RULE_COUNTER_REPLAY;
      }
      return r;
}

/*
 * Integrity / anti-spoof tier (WARN). Guards against a claimed position that
 * is inconsistent with observed RSSI: using the free-space path-loss model
 * (KX_RSSI_AT_1M / KX_PATHLOSS_N), a signal stronger than expected-plus-
 * KX_RSSI_MARGIN_DB for the claimed distance implies the drone is actually
 * much closer than it says (possible spoofed GPS). Requires a configured
 * fixed sensor position (th->has_sensor_pos), otherwise this is a no-op.
 */
kx_result_t magpie_validate_rssi_position(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (table == NULL || track == NULL) return r;

    int flag = 0;
    if (track == NULL || th == NULL) return r;
    if (!th->has_sensor_pos) return r;
    if (!track->has_location) return r;
    if (track->location.Latitude == 0.0 && track->location.Longitude == 0.0) return r;
    if (track->last_rssi_dbm == 0) return r;
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

/*
 * Integrity / anti-spoof tier (WARN). Guards against a position update that
 * implies faster-than-possible travel since the previous fix — a sharper,
 * sensor-relative variant of magpie_validate_jump. Also requires a configured
 * fixed sensor position (th->has_sensor_pos).
 */
kx_result_t magpie_validate_kinematic_teleport(const track_table_t *table, const track_t *track, const magpie_thresholds_t *th)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (table == NULL || track == NULL) return r;
    int flag = 0;
    if (!th->has_sensor_pos) return r;
    if (!track->has_location) return r;
    if (track->location.Latitude == 0.0 && track->location.Longitude == 0.0) return r;

    double distance_km = jump_detection(track);
    double elapsed_secs = (double)(track->last_location_seen - track->prev_location_seen);
    if (elapsed_secs <= 0.0) return r;

    if ((distance_km * 3600.0 / elapsed_secs) > th->max_speed_kmh){
        flag = 1;
    }

    if (flag == 1){
        r.rule = KX_RULE_IMPLAUSIBLE_KINEMATIC_TELEPORT;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

/* ANSI/CTA-2063-A serial-number format: 4 alnum manufacturer code chars, a
 * length digit/hex-digit (1-9, A-F) giving the serial length, then that many
 * chars from [0-9A-Z] excluding easily-confused O and I. */
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

/* Loose UTM-assigned/CAA registration format: 1-20 chars of alnum, '-', '.'. */
static bool is_valid_caa_registration(const char *id)
{
    size_t len = strlen(id);
    if (len == 0 || len > 20) return false;
    for (size_t i = 0; i < len; i++){
        unsigned char c = id[i];
        if (!(isalnum(c) || c == '-' || c == '.')){
            return false;
        }
    }
    return true;
}

/*
 * P4 rule pack (WARN). Guards against a UAS ID that doesn't match the format
 * required for its declared IDType — serial numbers must be CTA-2063-A,
 * UTM-assigned UUIDs must look like a CAA registration. Other ID types have
 * no defined format here and are not checked.
 */
kx_result_t magpie_validate_uasid_format(const droneID_basicID_data *data)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (data == NULL) return r;
    bool ok = true;
    switch(data->IDType){
        case DRONE_ID_IDTYPE_SERIAL_NUMBER:
            ok = is_valid_cta2063a(data->UASID);
            break;
        case DRONE_ID_IDTYPE_UTM_ASSIGNED_UUID:
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

/* Operator ID format: 3-letter country/issuer prefix (A-Z) followed by 1-17
 * alnum/'-' chars, 4-20 chars total. */
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

/*
 * P4 rule pack (WARN). Guards against an Operator ID that isn't a valid
 * registration format (see is_valid_operator_id) — a malformed operator ID
 * can't be traced back to a real registrant.
 */
kx_result_t magpie_validate_operator_id_format(const OperatorID_data *data)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (data == NULL) return r;
    if (!is_valid_operator_id(data->OperatorId)){
        r.rule = KX_RULE_OPERATOR_ID_FORMAT;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

/*
 * EU 2019/945 category/class consistency table: Open category only permits
 * class markings C0-C4; Certified category has no class-marking scheme
 * (any declared class is inconsistent); Specific and Undeclared are
 * unconstrained here.
 */
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

/*
 * P4 rule pack (WARN). Guards against a System message declaring an EU
 * category/class combination that EU 2019/945 doesn't allow (see
 * is_valid_eu_class).
 */
kx_result_t magpie_validate_eu_class_consistency(const sys_data *data)
{
    kx_result_t r = { .rule = KX_RULE_NONE, .severity = KX_SEVERITY_ERROR };
    if (data == NULL) return r;
    if (!is_valid_eu_class(data->CategoryEU, data->ClassEU)){
        r.rule = KX_RULE_EU_CLASS_CONSISTENCY;
        r.severity = KX_SEVERITY_WARN;
    }
    return r;
}

/*
 * Composes every check into one bitmask. Track/table-level checks always run;
 * message-shape checks (basic_id, location, operator_id, EU class) only run
 * for the message type that actually arrived, since msg->data is a union.
 * Step 3 of the "adding a rule" recipe (CLAUDE.md): a rule not OR'd in here
 * is dead — it compiles but never fires.
 */
uint32_t magpie_validate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter)
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

/*
 * Public entry point: runs magpie_validate(), applies the profile's
 * rule-pack gate (disabled_rules masked out before findings are built — a
 * disabled rule produces no finding at all, not a suppressed one), expands
 * the surviving bits into findings via rule_meta(), and derives compliance
 * from KX_RULE_ERROR_MASK only. finding_count is capped at KX_MAX_FINDINGS;
 * remaining tripped bits still count toward `flags`/compliance but won't get
 * a findings[] entry.
 */
kx_verdict_t magpie_evaluate(const track_t *track, const decoded_message_t *msg, time_t now, const magpie_thresholds_t *th, const track_table_t *table, const uint8_t *msg_counter, const magpie_profile_t *profile){
    kx_verdict_t verdict = {
        .flags = KX_RULE_NONE,
        .compliant = true,
        .ruleset_version = KX_RULESET_VERSION,
        .finding_count = 0
    };
    if (track == NULL) return verdict;

    verdict.flags = magpie_validate(track, msg, now, th, table, msg_counter) & ~profile->disabled_rules;

    size_t i;
    for (i = 0; i < sizeof(all) / sizeof(all[0]) && verdict.finding_count < KX_MAX_FINDINGS; i++){
        if (verdict.flags & all[i]){
            kx_finding_t *f = &verdict.findings[verdict.finding_count++];
            f->rule = all[i];
            rule_meta(all[i], &f->severity, &f->field, &f->reg_ref, &f->detail);
        }
    }
    /* Invariant: only KX_RULE_ERROR_MASK bits flip compliance. WARN-severity
     * findings (jump, reserved bits, all plausibility/integrity rules) are
     * reported but never make a track non-compliant. */
    verdict.compliant = (verdict.flags & KX_RULE_ERROR_MASK) == 0;
    return verdict;
}

