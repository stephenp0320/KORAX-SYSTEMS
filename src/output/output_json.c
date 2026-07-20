#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"

int track_to_json(const track_t *track, char *buffer, size_t len){;
    if (track == NULL || buffer == NULL || len == 0){
        return -1;
    }
    int write_json = snprintf(buffer, len,
          "{"
          "\"schema_version\":1,"
          "\"uas_id\":\"%s\","
          "\"operator_id\":\"%s\","
          "\"position\":{\"lat\":%.7f,\"lon\":%.7f,\"alt_geo_m\":%.1f,\"alt_baro_m\":%.1f},"
          "\"velocity\":{\"speed_mps\":%.2f,\"vspeed_mps\":%.2f,\"heading_deg\":%.1f},"
          "\"timestamp_s\":%.1f,"
          "\"compliant\":%s,"
          "\"flags\":%u"
        "}",
        track->uas_id,
        track->has_operator_id ? track->operator_id.OperatorId : "",
        track->location.Latitude,  track->location.Longitude,
        track->location.AltitudeGeo, track->location.AltitudeBaro,
        track->location.SpeedHorizontal, track->location.SpeedVertical,
        track->location.Direction,
        track->location.TimeStamp,
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
