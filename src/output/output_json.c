/* JSON serialisation for the pipeline's output stage: tracks and Magpie
 * verdicts -> NDJSON lines consumed by server.c (and MQTT). This is the
 * public wire contract — field names, nesting, and rule/severity strings
 * are pinned byte-for-byte by test_golden_verdict. Any intentional change
 * to the shape emitted here must update that golden string in the same
 * commit; don't rename or restructure "for tidiness". */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/output_json.h"
#include "korax/magpie_validator.h"
/* Scratch buffer for the rendered findings array; must comfortably outsize
 * the largest findings[] a track can carry, or findings_to_json() fails and
 * track_to_json() returns -2, silently dropping the update (see daemon
 * line/track buffer sizing, which must stay >= this). */
#define FBUF_SIZE 1024

/* Escapes src into dst for embedding in a JSON string. Truncates safely
 * (never overflows dst) rather than erroring if src doesn't fit. */
static void json_escape(const char *src, char *dst, size_t dstlen){
    size_t j = 0;
    if (dstlen == 0){
        return;
    }

    for (size_t i = 0; src[i] != '\0'; i++){
        unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\'){
            if (j + 2 >= dstlen){
                break;
            }
            dst[j++] = '\\';
            dst[j++] = (char)c;
        } else if (c < 0x20){
            if (j + 6 >= dstlen){
                break;
            }
            j += (size_t)snprintf(dst + j, dstlen - j, "\\u%04x", c);
        } else {
            if (j + 1 >= dstlen){
                break;
            }
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
}

/* LANDMINE: must stay in lockstep with the kx_rule_t enum in
 * magpie_types.h. A rule added there without a case here doesn't fail to
 * build — it silently falls through to "unknown" in the JSON output. */
static const char *rule_name(kx_rule_t rule)
{
    switch(rule){
        case KX_RULE_MISSING_REQUIRED_MSG:
            return "missing_required_msg";
        case KX_RULE_NO_LOCATION_IN_GRACE:
            return "no_location_in_grace";
        case KX_RULE_RESERVED_BITS_SET:
            return "reserved_bits_set";
        case KX_RULE_FIELD_OUT_OF_RANGE:
            return "field_out_of_range";
        case KX_RULE_IMPLAUSIBLE_JUMP:
            return "implausible_jump";
        case KX_RULE_TIMESTAMP_REWIND:
            return "timestamp_rewind";
        case KX_RULE_BROADCAST_RATE:
            return "broadcast_rate";
        case KX_RULE_SPEED_EXCEEDS_TYPE:
            return "speed_exceeds_type";
        case KX_RULE_ALT_ENVELOPE:
            return "alt_envelope";
        case KX_RULE_OPERATOR_DISTANCE:
            return "operator_distance";
        case KX_RULE_COUNTER_REPLAY:
            return "counter_replay";
        case KX_RULE_IMPLAUSIBLE_KINEMATIC_TELEPORT:
            return "kinematic_teleport";
        case KX_RULE_MAX_CLIMB_MS:
            return "max_climb_ms";
        case KX_RULE_DUPLICATION_DETECTED:
            return "duplication_detected";
        case KX_RULE_ID_CHURN:
            return "id_churn";
        case KX_RULE_RSSI_POSITION:
            return "rssi_position";
        case KX_RULE_CLIMB_RATE:
            return "climb_rate";
        case KX_RULE_UASID_FORMAT:
            return "uasid_format";
        case KX_RULE_OPERATOR_ID_FORMAT:
            return "operator_id_format";
        case KX_RULE_EU_CLASS_CONSISTENCY:
            return "eu_class_consistency";
        default:
            return "unknown";
    }
}

static const char *sev_name(kx_severity_t severity){
    switch(severity){
        case KX_SEVERITY_ERROR:
            return "error";
        case KX_SEVERITY_WARN:
            return "warn";
        default:
            return "info";
    }
}

/* Renders the findings[] array as JSON; returns -1 if buf is too small
 * (snprintf truncation), else the written length. Callers treat -1 as
 * "buffer too small" and propagate their own -2 sentinel to callers. */
static int findings_to_json(const kx_finding_t *f, uint8_t count, char *buf, size_t len){
    int n = snprintf(buf, len, "[");
    if (n < 0 || (size_t)n >= len) return -1;

    for (uint8_t i = 0; i < count; i++){
        int m = snprintf(buf + n, len - n,
            "%s{\"rule\":\"%s\",\"severity\":\"%s\",\"field\":\"%s\",\"reg\":\"%s\",\"detail\":\"%s\"}",
            i ? "," : "",
            rule_name(f[i].rule),
            sev_name(f[i].severity),
            f[i].field,
            f[i].reg_ref,
            f[i].detail);

        if (m < 0 || (size_t)(n + m) >= len) return -1;
        n += m;
    }

    int m = snprintf(buf + n, len - n, "]");
    if (m < 0 || (size_t)(n + m) >= len) return -1;
    return n + m;
}

/* Contract: emits exactly the fields/nesting pinned by test_golden_verdict,
 * including the "validation":{compliant,ruleset_version,flags,findings}
 * block. -1 = bad args/encoding error; -2 = buffer too small (caller's
 * buffer must stay large enough for a full findings[] — see FBUF_SIZE and
 * the daemon's track/line buffer sizes) — a -2 here means the update was
 * silently dropped upstream. */
int track_to_json(const track_t *track, char *buffer, size_t len){;
    if (track == NULL || buffer == NULL || len == 0) return -1;

    char uas_esc[160];
    char op_esc[160];
    json_escape(track->uas_id, uas_esc, sizeof uas_esc);
    json_escape(track->has_operator_id ? track->operator_id.OperatorId : "", op_esc, sizeof op_esc);

    char _timestamp[24];
    if (track->location.TimeStamp < 0.0f){
        snprintf(_timestamp, sizeof _timestamp, "null");
    } else {
        snprintf(_timestamp, sizeof _timestamp, "%.1f", track->location.TimeStamp);
    }

    char fbuf[FBUF_SIZE];
    /* -2: findings didn't fit FBUF_SIZE -- update dropped, not corrupted. */
    if (findings_to_json(track->findings, track->finding_count, fbuf, sizeof(fbuf)) < 0) return -2;

    int write_json = snprintf(buffer, len,
         "{"
          "\"schema_version\":%d,"
          "\"uas_id\":\"%s\","
          "\"operator_id\":\"%s\","
          "\"position\":{\"lat\":%.7f,\"lon\":%.7f,\"alt_geo_m\":%.1f,\"alt_baro_m\":%.1f},"
          "\"velocity\":{\"speed_mps\":%.2f,\"vspeed_mps\":%.2f,\"heading_deg\":%.1f},"
          "\"timestamp_s\":%s,"
              "\"validation\":{"
              "\"compliant\":%s,"
              "\"ruleset_version\":%u,"
              "\"flags\":%u,"
              "\"findings\":%s"
          "}"
        "}",
        KX_JSON_SCHEMA_VERSION,
        uas_esc,
        op_esc,
        track->location.Latitude,  track->location.Longitude,
        track->location.AltitudeGeo, track->location.AltitudeBaro,
        track->location.SpeedHorizontal, track->location.SpeedVertical,
        track->location.Direction,
        _timestamp,
        track->is_compliant ? "true" : "false",
        (unsigned)KX_RULESET_VERSION,
        (unsigned)track->flags,
        fbuf);

    if (write_json < 0) return -1;
    if (write_json >= (int)len) return -2; /* caller's buffer too small; update dropped */
    return write_json;
}

/* Same -1/-2 convention as track_to_json, scoped to just the validation
 * block (used where callers already have the rest of the track envelope). */
int verdict_to_json(kx_verdict_t *verdict, uint8_t count, char *buffer, size_t len){;
    if (verdict == NULL || buffer == NULL || len == 0) return -1;
    char temp[KX_MAX_DETAILS];
    char fbuf[512];
    if (findings_to_json(verdict->findings, count, fbuf, sizeof fbuf) < 0) return -1;

    int write_json = snprintf(temp, sizeof(temp),
          "{"
              "\"validation\":{"
                  "\"compliant\":%s,"
                  "\"ruleset_version\":%u,"
                  "\"flags\":%u,"
                  "\"findings\":%s"
              "}"
          "}",
        verdict->compliant ? "true" : "false",
        (unsigned)verdict->ruleset_version,
        (unsigned)verdict->flags, fbuf);

    if (write_json < 0 || (size_t)write_json >= sizeof(temp)) return -1;
    if ((size_t)write_json >= len) return -2;
    memcpy(buffer, temp, (size_t)write_json + 1);
    return write_json;
}
