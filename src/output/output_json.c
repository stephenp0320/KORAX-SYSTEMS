#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/output_json.h"

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

int track_to_json(const track_t *track, char *buffer, size_t len){;
    if (track == NULL || buffer == NULL || len == 0){
        return -1;
    }
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

    int write_json = snprintf(buffer, len,
         "{"
          "\"schema_version\":%d,"
          "\"uas_id\":\"%s\","
          "\"operator_id\":\"%s\","
          "\"position\":{\"lat\":%.7f,\"lon\":%.7f,\"alt_geo_m\":%.1f,\"alt_baro_m\":%.1f},"
          "\"velocity\":{\"speed_mps\":%.2f,\"vspeed_mps\":%.2f,\"heading_deg\":%.1f},"
          "\"timestamp_s\":%s,"
          "\"compliant\":%s,"
          "\"flags\":%u"
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
        (unsigned)track->flags);
    if (write_json < 0){
        return -1;
    }
    if (write_json > (int)len){
        return -2;
    }
    return write_json;
}
