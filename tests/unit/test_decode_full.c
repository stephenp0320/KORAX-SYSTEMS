#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"
#include "korax/output_json.h"
#include "korax/server.h"
#include "daemon/config.h"
#include "daemon/log.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg)                                               \
    do { tests_run++;                                                        \
        if (!(cond)) { tests_failed++;                                       \
            printf("FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

#define ASSERT_EQ_INT(actual, expected, msg)                                 \
    do { tests_run++;                                                        \
        long long a_ = (long long)(actual), e_ = (long long)(expected);      \
        if (a_ != e_) { tests_failed++;                                      \
            printf("FAIL: %s (line %d) expected %lld got %lld\n",            \
                   msg, __LINE__, e_, a_); } } while (0)

#define ASSERT_EQ_STR(actual, expected, msg)                                 \
    do { tests_run++;                                                        \
        if (strcmp((actual), (expected)) != 0) { tests_failed++;            \
            printf("FAIL: %s (line %d) expected '%s' got '%s'\n",            \
                   msg, __LINE__, (expected), (actual)); } } while (0)

#define ASSERT_NEAR(actual, expected, tol, msg)                              \
    do { tests_run++;                                                        \
        double d_ = (double)(actual) - (double)(expected);                   \
        if (d_ < 0) d_ = -d_;                                                \
        if (d_ > (tol)) { tests_failed++;                                    \
            printf("FAIL: %s (line %d) expected %.7f got %.7f\n",            \
                   msg, __LINE__, (double)(expected), (double)(actual)); } } while (0)

/* ---- Real ODID wire fixtures (generated, protocol version 2) ---- */

/* Basic ID: UAType=2 (multirotor), IDType=1 (serial), UASID="1581F5BJD248Q00A8XVX" */
static const uint8_t PKT_BASIC[25] = { 0x02, 0x12, 0x31, 0x35, 0x38, 0x31, 0x46, 0x35, 0x42, 0x4A, 0x44, 0x32, 0x34, 0x38, 0x51, 0x30, 0x30, 0x41, 0x38, 0x58, 0x56, 0x58, 0x00, 0x00, 0x00 };

/* Location: airborne, HeightType=over-ground, Direction=215, SpeedH=12.0, SpeedV=-3.0,
   Lat=51.4778, Lon=-0.0014, AltBaro=120, AltGeo=125.5, Height=60,
   HorizAcc=11, VertAcc=6, BaroAcc=6, SpeedAcc=4, TSAcc=2, TimeStamp=1234.5 */
static const uint8_t PKT_LOCATION[25] = { 0x12, 0x26, 0x23, 0x30, 0xFA, 0x90, 0xE3, 0xAE, 0x1E, 0x50, 0xC9, 0xFF, 0xFF, 0xC0, 0x08, 0xCB, 0x08, 0x48, 0x08, 0x6B, 0x64, 0x39, 0x30, 0x02, 0x00 };

/* Auth page 0: AuthType=1, DataPage=0, LastPageIndex=2, Length=63, TS=0x12345678, AuthData=0x10..0x20 */
static const uint8_t PKT_AUTH[25] = { 0x22, 0x10, 0x02, 0x3F, 0x78, 0x56, 0x34, 0x12, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };

/* Self ID: DescType=1 (emergency), Desc="Inspection Flight" */
static const uint8_t PKT_SELFID[25] = { 0x32, 0x01, 0x49, 0x6E, 0x73, 0x70, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x46, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* System: OpLocType=2(fixed), Classification=1(EU), OpLat=52.3, OpLon=4.9, AreaCount=1,
   AreaRadius=500, Ceiling=150, Floor=20, CategoryEU=1(open), ClassEU=2(class1),
   OpAltGeo=105, Timestamp=12345678 */
static const uint8_t PKT_SYSTEM[25] = { 0x42, 0x06, 0xC0, 0x58, 0x2C, 0x1F, 0x40, 0xAE, 0xEB, 0x02, 0x01, 0x00, 0x32, 0xFC, 0x08, 0xF8, 0x07, 0x12, 0xA2, 0x08, 0x4E, 0x61, 0xBC, 0x00, 0x00 };

/* Operator ID: OperatorIdType=0, OperatorId="GBR-OP-12345678ABCDE" */
static const uint8_t PKT_OPID[25] = { 0x52, 0x00, 0x47, 0x42, 0x52, 0x2D, 0x4F, 0x50, 0x2D, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x41, 0x42, 0x43, 0x44, 0x45, 0x00, 0x00, 0x00 };

/* Message Pack: header (type F, SingleMessageSize=25, count=3) + BasicID, Location, System */
static const uint8_t PKT_PACK[78] = { 0xF2, 0x19, 0x03, 0x02, 0x12, 0x31, 0x35, 0x38, 0x31, 0x46, 0x35, 0x42, 0x4A, 0x44, 0x32, 0x34, 0x38, 0x51, 0x30, 0x30, 0x41, 0x38, 0x58, 0x56, 0x58, 0x00, 0x00, 0x00, 0x12, 0x26, 0x23, 0x30, 0xFA, 0x90, 0xE3, 0xAE, 0x1E, 0x50, 0xC9, 0xFF, 0xFF, 0xC0, 0x08, 0xCB, 0x08, 0x48, 0x08, 0x6B, 0x64, 0x39, 0x30, 0x02, 0x00, 0x42, 0x06, 0xC0, 0x58, 0x2C, 0x1F, 0x40, 0xAE, 0xEB, 0x02, 0x01, 0x00, 0x32, 0xFC, 0x08, 0xF8, 0x07, 0x12, 0xA2, 0x08, 0x4E, 0x61, 0xBC, 0x00, 0x00 };

static void test_string_helpers(void)
{
    ASSERT_EQ_STR(uatype_to_str(DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR), "UATYPE_HELICOPTER_OR_MULTIROTOR", "uatype multirotor");
    ASSERT_EQ_STR(uatype_to_str((DRONE_ID_uatype_t)99), "UATYPE_UNKNOWN", "uatype unknown");
    ASSERT_EQ_STR(idtype_to_str(DRONE_ID_IDTYPE_SERIAL_NUMBER), "IDTYPE_SERIAL_NUMBER", "idtype serial");
    ASSERT_EQ_STR(msg_type_to_str(MESSAGETYPE_LOCATION), "MESSAGETYPE_LOCATION", "msgtype location");
    ASSERT_EQ_STR(status_to_str(DRONE_ID_STATUS_AIRBORNE), "STATUS_AIRBORNE", "status airborne");
    ASSERT_EQ_STR(auth_type_to_str(AUTH_UAS_ID_SIGNATURE), "AUTH_UAS_ID_SIGNATURE", "auth uas sig");
}

static void test_basic_id(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_BASIC, 25, &m), DECODE_OK, "basic id decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_BASIC_ID, "basic id type");
    ASSERT_EQ_INT(m.data.basic_id.UAType, DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR, "basic UAType");
    ASSERT_EQ_INT(m.data.basic_id.IDType, DRONE_ID_IDTYPE_SERIAL_NUMBER, "basic IDType");
    ASSERT_EQ_STR(m.data.basic_id.UASID, "1581F5BJD248Q00A8XVX", "basic UASID");
}

static void test_location(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_LOCATION, 25, &m), DECODE_OK, "location decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_LOCATION, "location type");
    DRONE_ID_Location_data *L = &m.data.location;
    ASSERT_EQ_INT(L->Status, DRONE_ID_STATUS_AIRBORNE, "loc status");
    ASSERT_EQ_INT(L->HeightType, DRONE_ID_HEIGHT_REF_OVER_GROUND, "loc height type");
    ASSERT_NEAR(L->Direction, 215.0, 0.5, "loc direction (E/W bit)");
    ASSERT_NEAR(L->SpeedHorizontal, 12.0, 0.01, "loc speed horizontal");
    ASSERT_NEAR(L->SpeedVertical, -3.0, 0.01, "loc speed vertical (signed)");
    ASSERT_NEAR(L->Latitude, 51.4778, 1e-6, "loc latitude (LE)");
    ASSERT_NEAR(L->Longitude, -0.0014, 1e-6, "loc longitude (LE, negative)");
    ASSERT_NEAR(L->AltitudeBaro, 120.0, 0.01, "loc alt baro");
    ASSERT_NEAR(L->AltitudeGeo, 125.5, 0.01, "loc alt geo");
    ASSERT_NEAR(L->Height, 60.0, 0.01, "loc height (2-byte)");
    ASSERT_EQ_INT(L->HorizAccuracy, 11, "loc horiz accuracy");
    ASSERT_EQ_INT(L->VertAccuracy, 6, "loc vert accuracy");
    ASSERT_EQ_INT(L->BaroAccuracy, 6, "loc baro accuracy");
    ASSERT_EQ_INT(L->SpeedAccuracy, 4, "loc speed accuracy");
    ASSERT_EQ_INT(L->TSAccuracy, 2, "loc ts accuracy");
    ASSERT_NEAR(L->TimeStamp, 1234.5, 0.05, "loc timestamp (LE, /10)");
}

static void test_auth(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_AUTH, 25, &m), DECODE_OK, "auth decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_AUTH, "auth type");
    authentication_data *A = &m.data.auth;
    ASSERT_EQ_INT(A->msg_type, MESSAGETYPE_AUTH, "auth msg_type");
    ASSERT_EQ_INT(A->auth_type, AUTH_UAS_ID_SIGNATURE, "auth auth_type (high nibble)");
    ASSERT_EQ_INT(A->data_page, 0, "auth data_page (low nibble)");
    ASSERT_EQ_INT(A->last_page_idx, 2, "auth last_page_idx");
    ASSERT_EQ_INT(A->length, 63, "auth length");
    ASSERT_EQ_INT(A->timestamp, 0x12345678, "auth timestamp (LE u32)");
    ASSERT_EQ_INT(A->AuthData[0], 0x10, "auth data first byte (offset 8)");
    ASSERT_EQ_INT(A->AuthData[16], 0x20, "auth data 17th byte");
}

static void test_self_id(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_SELFID, 25, &m), DECODE_OK, "self id decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_SELF_ID, "self id type");
    ASSERT_EQ_INT(m.data.self_id.DescType, DESC_TYPE_EMERGENCY, "self id desc type");
    ASSERT_EQ_STR(m.data.self_id.desc, "Inspection Flight", "self id desc");
}

static void test_system(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_SYSTEM, 25, &m), DECODE_OK, "system decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_SYSTEM, "system type");
    sys_data *S = &m.data.system;
    ASSERT_EQ_INT(S->OperatorLocType, OP_LOCATION_TYPE_FIXED, "sys operator loc type");
    ASSERT_EQ_INT(S->ClassificationType, CLASSIFICATION_TYPE_EU, "sys classification type");
    ASSERT_NEAR(S->OperatorLatitude, 52.3, 1e-6, "sys operator latitude");
    ASSERT_NEAR(S->OperatorLongitude, 4.9, 1e-6, "sys operator longitude");
    ASSERT_EQ_INT(S->AreaCount, 1, "sys area count (u16)");
    ASSERT_EQ_INT(S->AreaRadius, 500, "sys area radius (x10)");
    ASSERT_NEAR(S->AreaCeiling, 150.0, 0.01, "sys area ceiling");
    ASSERT_NEAR(S->AreaFloor, 20.0, 0.01, "sys area floor");
    ASSERT_EQ_INT(S->CategoryEU, CATEGORY_EU_OPEN, "sys category EU (high nibble)");
    ASSERT_EQ_INT(S->ClassEU, CLASS_EU_CLASS_1, "sys class EU (low nibble)");
    ASSERT_NEAR(S->OperatorAltitudeGeo, 105.0, 0.01, "sys operator altitude geo");
    ASSERT_EQ_INT(S->Timestamp, 12345678, "sys timestamp (LE u32)");
}

static void test_operator_id(void)
{
    decoded_message_t m;
    ASSERT_EQ_INT(decode_message(PKT_OPID, 25, &m), DECODE_OK, "operator id decodes");
    ASSERT_EQ_INT(m.type, MESSAGETYPE_OPERATOR_ID, "operator id type");
    ASSERT_EQ_INT(m.data.operator_id.OperatorIdType, ODID_OPERATOR_ID, "op id type");
    ASSERT_EQ_STR(m.data.operator_id.OperatorId, "GBR-OP-12345678ABCDE", "operator id string");
}

static void test_error_paths(void)
{
    decoded_message_t m;
    uint8_t short_pkt[10] = { 0x12, 0x26, 0x23 };
    uint8_t bad_type[25]  = { 0xE0 };

    ASSERT_EQ_INT(decode_message(NULL, 25, &m), DECODE_ERR_NULL, "null packet -> NULL");
    ASSERT_EQ_INT(decode_message(PKT_LOCATION, 25, NULL), DECODE_ERR_NULL, "null out -> NULL");
    ASSERT_EQ_INT(decode_message(short_pkt, 10, &m), DECODE_ERR_TOO_SHORT, "short -> TOO_SHORT");
    ASSERT_EQ_INT(decode_message(bad_type, 25, &m), DECODE_ERR_BAD_TYPE, "bad type -> BAD_TYPE");
    ASSERT_EQ_INT(decode_message(PKT_PACK, 25, &m), DECODE_ERR_BAD_TYPE, "pack as single -> BAD_TYPE");
}

static void test_message_pack_valid(void)
{
    track_table_t table;
    track_table_init(&table);

    ASSERT_EQ_INT(decode_message_pack((uint8_t *)PKT_PACK, sizeof(PKT_PACK), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_OK, "pack decodes");
    ASSERT_EQ_INT(table.num_tracks, 1, "pack -> one track");
    ASSERT_EQ_INT(table.tracks[0].has_basic_id, 1, "pack track has basic id");
    ASSERT_EQ_INT(table.tracks[0].has_location, 1, "pack track has location");
    ASSERT_EQ_INT(table.tracks[0].has_system, 1, "pack track has system");
    ASSERT_EQ_STR(table.tracks[0].uas_id, "1581F5BJD248Q00A8XVX", "pack track uas id");
    ASSERT_NEAR(table.tracks[0].location.Latitude, 51.4778, 1e-6, "pack location latitude");
}

static void test_message_pack_errors(void)
{
    track_table_t table;

    uint8_t zero_count[3] = { 0xF2, 0x19, 0x00 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(zero_count, sizeof(zero_count), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_RANGE, "pack count 0 -> RANGE");

    uint8_t big_count[3] = { 0xF2, 0x19, 0xFF };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(big_count, sizeof(big_count), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_RANGE, "pack count 255 -> RANGE");

    uint8_t bad_size[3] = { 0xF2, 0x18, 0x03 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(bad_size, sizeof(bad_size), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_RANGE, "pack bad single size -> RANGE");

    uint8_t truncated[10] = { 0xF2, 0x19, 0x02, 0x02, 0x12, 0x31 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(truncated, sizeof(truncated), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_TOO_SHORT, "pack truncated -> TOO_SHORT");

    uint8_t tiny[2] = { 0xF2, 0x19 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(tiny, sizeof(tiny), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_TOO_SHORT, "pack tiny header -> TOO_SHORT");

    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack((uint8_t *)PKT_LOCATION, sizeof(PKT_LOCATION), &table, &MAGPIE_DEFAULT_THRESHOLDS), DECODE_ERR_BAD_TYPE, "non-pack -> BAD_TYPE");
}

static void test_jump_distance(void){
    track_t track;
    memset(&track, 0, sizeof(track));

    track.prev_location.Latitude = 51.4778;
    track.prev_location.Longitude = -0.0014;

    track.location.Latitude = 51.4788;
    track.location.Longitude = -0.0014;

    double distance_km = jump_detection(&track);

    ASSERT_NEAR(
        distance_km,
        0.111,
        0.005,
        "jump distance approximately 111 metres"
    );
}

static void test_previous_location_tracking(void){
    track_t track;
    memset(&track, 0, sizeof(track));

    decoded_message_t first = {0};
    first.type = MESSAGETYPE_LOCATION;
    first.data.location.Latitude  = 51.4778;
    first.data.location.TimeStamp = 100.0;
    decoded_message_t second = {0};
    second.type = MESSAGETYPE_LOCATION;
    second.data.location.Latitude  = 51.4788;
    second.data.location.TimeStamp = 101.0;

    track_table_update(&track, &first);
    track_table_update(&track, &second);

    ASSERT_EQ_INT(track.msg_count_location, 2, "two locations counted");
    ASSERT_NEAR(track.prev_location.Latitude,  51.4778, 1e-6, "previous latitude retained");
    ASSERT_NEAR(track.location.Latitude,       51.4788, 1e-6, "current latitude updated");
    ASSERT_NEAR(track.prev_location.TimeStamp, 100.0, 0.001, "previous ODID timestamp retained");
    ASSERT_NEAR(track.location.TimeStamp,      101.0, 0.001, "current ODID timestamp updated");
}

static void test_first_location_has_no_previous(void){
    track_t track;
    memset(&track, 0, sizeof(track));

    decoded_message_t first = {0};
    first.type = MESSAGETYPE_LOCATION;
    first.data.location.Latitude  = 51.4778;
    first.data.location.TimeStamp = 100.0;
    track_table_update(&track, &first);

    ASSERT_EQ_INT(track.msg_count_location, 1, "one location counted");
    ASSERT_NEAR(track.location.Latitude,       51.4778, 1e-6, "current latitude set");
    ASSERT_NEAR(track.prev_location.Latitude,  0.0, 1e-9, "no previous latitude yet");
    ASSERT_NEAR(track.prev_location.TimeStamp, 0.0, 1e-9, "no previous ODID timestamp yet");
    ASSERT_EQ_INT(track.prev_location_seen,    0,        "no previous receive time yet");
}

static void test_previous_location_rolls_forward(void){
    track_t track;
    memset(&track, 0, sizeof(track));
    decoded_message_t m1 = {0}, m2 = {0}, m3 = {0};
    m1.type = m2.type = m3.type = MESSAGETYPE_LOCATION;
    m1.data.location.Latitude = 51.0; m1.data.location.TimeStamp = 100.0;
    m2.data.location.Latitude = 52.0; m2.data.location.TimeStamp = 101.0;
    m3.data.location.Latitude = 53.0; m3.data.location.TimeStamp = 102.0;
    track_table_update(&track, &m1);
    track_table_update(&track, &m2);
    track_table_update(&track, &m3);

    ASSERT_EQ_INT(track.msg_count_location, 3, "three locations counted");
    ASSERT_NEAR(track.prev_location.Latitude,  52.0, 1e-6, "prev is the 2nd fix");
    ASSERT_NEAR(track.location.Latitude,       53.0, 1e-6, "current is the 3rd fix");
    ASSERT_NEAR(track.prev_location.TimeStamp, 101.0, 0.001, "prev ODID ts is the 2nd");
    ASSERT_NEAR(track.location.TimeStamp,      102.0, 0.001, "current ODID ts is the 3rd");
}



static void test_magpie_required_missing(void){
    /* positive: a track missing Location is flagged */
    track_t t; memset(&t, 0, sizeof t);
    t.has_basic_id = 1; t.has_system = 1; t.has_operator_id = 1;   /* no location */
    kx_result_t r = magpie_validate_track(&t, 1000, &MAGPIE_DEFAULT_THRESHOLDS);
    ASSERT_TRUE(r.rule & KX_RULE_MISSING_REQUIRED_MSG, "missing Location -> flagged");
    ASSERT_EQ_INT(r.severity, KX_SEVERITY_ERROR, "missing required -> ERROR severity");

    /* negative: a complete, fresh track is not flagged */
    track_t ok; memset(&ok, 0, sizeof ok);
    ok.has_basic_id = 1; ok.has_location = 1; ok.has_system = 1; ok.has_operator_id = 1;
    ok.last_location_seen = 1000;
    kx_result_t r2 = magpie_validate_track(&ok, 1000, &MAGPIE_DEFAULT_THRESHOLDS);
    ASSERT_EQ_INT(r2.rule, KX_RULE_NONE, "complete fresh track -> compliant");
}

static void test_magpie_grace_window(void){
    track_t t; memset(&t, 0, sizeof t);
    t.has_basic_id = 1; t.has_location = 1; t.has_system = 1; t.has_operator_id = 1;
    t.last_location_seen = 1000;

    kx_result_t stale = magpie_validate_track(&t, 1000 + 10, &MAGPIE_DEFAULT_THRESHOLDS);  /* 10s > 5s grace */
    ASSERT_TRUE(stale.rule & KX_RULE_NO_LOCATION_IN_GRACE, "stale location -> flagged");

    kx_result_t fresh = magpie_validate_track(&t, 1000 + 2, &MAGPIE_DEFAULT_THRESHOLDS);   /* within grace */
    ASSERT_TRUE((fresh.rule & KX_RULE_NO_LOCATION_IN_GRACE) == 0, "fresh location -> not flagged");
}

static void test_magpie_location_range(void){
    DRONE_ID_Location_data good; memset(&good, 0, sizeof good);
    good.Latitude = 51.0; good.Longitude = 0.0;
    good.AltitudeGeo = 100.0; good.AltitudeBaro = 100.0;
    good.TimeStamp = 10.0; good.Status = DRONE_ID_STATUS_AIRBORNE;
    ASSERT_EQ_INT(magpie_validate_location(&good).rule, KX_RULE_NONE, "valid location -> compliant");

    DRONE_ID_Location_data bad = good; bad.Latitude = 200.0;   /* out of range */
    ASSERT_TRUE(magpie_validate_location(&bad).rule & KX_RULE_FIELD_OUT_OF_RANGE, "bad latitude -> flagged");

    DRONE_ID_Location_data unknown = good; unknown.TimeStamp = -1.0f;  /* legal sentinel */
    ASSERT_TRUE((magpie_validate_location(&unknown).rule & KX_RULE_FIELD_OUT_OF_RANGE) == 0,
                "unknown timestamp (-1) -> not flagged");
}

static void test_magpie_basic_id(void){
    droneID_basicID_data good; memset(&good, 0, sizeof good);
    good.IDType = DRONE_ID_IDTYPE_SERIAL_NUMBER;
    good.UAType = DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    strcpy(good.UASID, "1581F5BJD248Q00A8XVX");
    ASSERT_EQ_INT(magpie_validate_basic_id(&good).rule, KX_RULE_NONE, "valid basic id -> compliant");

    droneID_basicID_data empty = good; empty.UASID[0] = '\0';
    ASSERT_TRUE(magpie_validate_basic_id(&empty).rule & KX_RULE_MISSING_REQUIRED_MSG, "empty UAS ID -> flagged");

    droneID_basicID_data bad = good; bad.IDType = (DRONE_ID_idtype_t)9;  /* reserved */
    ASSERT_TRUE(magpie_validate_basic_id(&bad).rule & KX_RULE_FIELD_OUT_OF_RANGE, "reserved IDType -> flagged");
}

static void test_magpie_jump(void){
    /* positive: ~111 km in 1 s is impossible */
    track_t fast; memset(&fast, 0, sizeof fast);
    fast.msg_count_location = 2;
    fast.prev_location.Latitude = 51.0; fast.prev_location.Longitude = 0.0;
    fast.location.Latitude      = 52.0; fast.location.Longitude      = 0.0;
    fast.prev_location_seen = 1000; fast.last_location_seen = 1001;
    ASSERT_TRUE(magpie_validate_jump(&fast, &MAGPIE_DEFAULT_THRESHOLDS).rule & KX_RULE_IMPLAUSIBLE_JUMP, "impossible speed -> flagged");

    /* negative: a small move over 10 s is fine */
    track_t slow; memset(&slow, 0, sizeof slow);
    slow.msg_count_location = 2;
    slow.prev_location.Latitude = 51.0000; slow.location.Latitude = 51.0001;
    slow.prev_location_seen = 1000; slow.last_location_seen = 1010;
    ASSERT_TRUE((magpie_validate_jump(&slow, &MAGPIE_DEFAULT_THRESHOLDS).rule & KX_RULE_IMPLAUSIBLE_JUMP) == 0, "normal move -> not flagged");

    /* only one fix: nothing to compare, not a violation */
    track_t single; memset(&single, 0, sizeof single);
    single.msg_count_location = 1;
    ASSERT_EQ_INT(magpie_validate_jump(&single, &MAGPIE_DEFAULT_THRESHOLDS).rule, KX_RULE_NONE, "single fix -> no jump check");
}

static void test_magpie_reserved_bits(void){
    /* Decode side: a clean location has no reserved bits. */
    decoded_message_t clean; memset(&clean, 0, sizeof clean);
    ASSERT_EQ_INT(decode_message(PKT_LOCATION, 25, &clean), DECODE_OK, "clean location decodes OK");
    ASSERT_EQ_INT(clean.reserved_bits, 0, "clean location -> reserved_bits 0");

    /* Set the Location flag-byte reserved bit (byte 1, bit 3) and re-decode.
     * The message must still decode OK (not dropped) but be marked. */
    uint8_t dirty[25]; memcpy(dirty, PKT_LOCATION, 25);
    dirty[1] |= 0x08;
    decoded_message_t marked; memset(&marked, 0, sizeof marked);
    ASSERT_EQ_INT(decode_message(dirty, 25, &marked), DECODE_OK, "reserved bit -> still decodes OK");
    ASSERT_EQ_INT(marked.reserved_bits, 1, "reserved bit -> reserved_bits 1");

    /* Rule side: a track that saw reserved bits is flagged WARN, not disqualified. */
    track_t t; memset(&t, 0, sizeof t);
    t.reserved_bits_seen = 1;
    kx_result_t r = magpie_validate_reserved_bitsets(&t);
    ASSERT_TRUE(r.rule & KX_RULE_RESERVED_BITS_SET, "reserved bits seen -> flagged");
    ASSERT_EQ_INT(r.severity, KX_SEVERITY_WARN, "reserved bits -> WARN severity");
    ASSERT_TRUE((r.rule & KX_RULE_ERROR_MASK) == 0, "reserved bits -> still compliant (not an ERROR)");

    /* Negative: no reserved bits seen -> not flagged. */
    track_t clean_t; memset(&clean_t, 0, sizeof clean_t);
    ASSERT_EQ_INT(magpie_validate_reserved_bitsets(&clean_t).rule, KX_RULE_NONE, "no reserved bits -> not flagged");
}

static void test_magpie_validate_composed(void){
    /* fully compliant track + valid location message -> 0 */
    track_t t; memset(&t, 0, sizeof t);
    t.has_basic_id = 1; t.has_location = 1; t.has_system = 1; t.has_operator_id = 1;
    t.last_location_seen = 5000; t.msg_count_location = 1;   /* <2 -> jump skipped */
    decoded_message_t msg; memset(&msg, 0, sizeof msg);
    msg.type = MESSAGETYPE_LOCATION;
    msg.data.location.Latitude = 51.0; msg.data.location.AltitudeGeo = 100.0;
    msg.data.location.AltitudeBaro = 100.0; msg.data.location.TimeStamp = 10.0;
    msg.data.location.Status = DRONE_ID_STATUS_AIRBORNE;
    ASSERT_EQ_INT(magpie_validate(&t, &msg, 5000, &MAGPIE_DEFAULT_THRESHOLDS), 0, "compliant track+msg -> 0 (compliant)");

    /* incomplete track -> at least the missing-required bit set */
    track_t bad; memset(&bad, 0, sizeof bad);
    bad.has_basic_id = 1;   /* missing location/system/operator */
    uint32_t flags = magpie_validate(&bad, NULL, 9999, &MAGPIE_DEFAULT_THRESHOLDS);
    ASSERT_TRUE(flags & KX_RULE_MISSING_REQUIRED_MSG, "incomplete track -> flagged by magpie_validate");
}

static void test_output_json(void){
    track_t t; memset(&t, 0, sizeof t);
    strcpy(t.uas_id, "1581F5BJD248Q00A8XVX");
    t.has_operator_id = 1;
    strcpy(t.operator_id.OperatorId, "GBR-OP-12345678ABCDE");
    t.location.Latitude = 52.6738; t.location.Longitude = -8.626;
    t.location.AltitudeGeo = 40.5; t.location.AltitudeBaro = 40.5;
    t.location.SpeedHorizontal = 10.0; t.location.Direction = 74.0;
    t.location.TimeStamp = 12.3;
    t.is_compliant = true; t.flags = 0;

    char buf[512];
    int n = track_to_json(&t, buf, sizeof buf);
    ASSERT_TRUE(n > 0, "serialise returns bytes written");
    ASSERT_TRUE(strstr(buf, "\"uas_id\":\"1581F5BJD248Q00A8XVX\"") != NULL, "uas_id present");
    ASSERT_TRUE(strstr(buf, "\"compliant\":true") != NULL, "compliant true present");
    ASSERT_TRUE(strstr(buf, "\"schema_version\":1") != NULL, "schema version present");

    /* Timestamp -1 sentinel -> JSON null, not -1.0 */
    t.location.TimeStamp = -1.0f;
    track_to_json(&t, buf, sizeof buf);
    ASSERT_TRUE(strstr(buf, "\"timestamp_s\":null") != NULL, "unknown timestamp -> null");

    /* Untrusted string with a quote must be escaped, not break the JSON */
    strcpy(t.uas_id, "bad\"id");
    track_to_json(&t, buf, sizeof buf);
    ASSERT_TRUE(strstr(buf, "bad\\\"id") != NULL, "embedded quote escaped");

    /* Buffer too small -> truncation reported as -2 */
    char tiny[16];
    ASSERT_EQ_INT(track_to_json(&t, tiny, sizeof tiny), -2, "small buffer -> -2 (truncated)");

    /* NULL args -> -1 */
    ASSERT_EQ_INT(track_to_json(NULL, buf, sizeof buf), -1, "null track -> -1");
}

/* Connect a blocking client socket to the server's Unix socket path. */
static int connect_client(const char *path){
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1){
        close(fd);
        return -1;
    }
    return fd;
}

static void test_server_feed(void){
    const char *path = "/tmp/korax_test.sock";
    server_t server;
    ASSERT_EQ_INT(server_init(&server, path), 0, "server_init succeeds");

    /* one live track, so the snapshot on connect has content */
    track_table_t table; memset(&table, 0, sizeof table);
    table.num_tracks = 1;
    strcpy(table.tracks[0].uas_id, "TESTDRONE123");
    table.tracks[0].is_compliant = true;

    char buf[1024];
    ssize_t n;

    /* client 1 connects; server accepts -> hello + snapshot */
    int c1 = connect_client(path);
    ASSERT_TRUE(c1 >= 0, "client 1 connects");
    ASSERT_EQ_INT(server_accept(&server, &table), 0, "accept client 1");
    n = read(c1, buf, sizeof buf - 1);
    ASSERT_TRUE(n > 0, "client 1 receives data");
    buf[(n > 0) ? (size_t)n : 0] = '\0';
    ASSERT_TRUE(strstr(buf, "\"event\":\"hello\"") != NULL, "client 1 gets hello");
    ASSERT_TRUE(strstr(buf, "\"event\":\"track_update\"") != NULL, "client 1 gets snapshot");
    ASSERT_TRUE(strstr(buf, "TESTDRONE123") != NULL, "snapshot carries the track");

    /* client 2 connects; server accepts -> its own hello + snapshot */
    int c2 = connect_client(path);
    ASSERT_TRUE(c2 >= 0, "client 2 connects");
    ASSERT_EQ_INT(server_accept(&server, &table), 0, "accept client 2");
    n = read(c2, buf, sizeof buf - 1);
    ASSERT_TRUE(n > 0, "client 2 receives data");
    buf[(n > 0) ? (size_t)n : 0] = '\0';
    ASSERT_TRUE(strstr(buf, "\"event\":\"hello\"") != NULL, "client 2 gets hello");

    /* broadcast reaches BOTH clients */
    const char *line = "{\"event\":\"track_expired\",\"uas_id\":\"TESTDRONE123\"}\n";
    ASSERT_EQ_INT(server_broadcast(&server, line, strlen(line)), 2, "broadcast delivered to 2 clients");
    n = read(c1, buf, sizeof buf - 1);
    buf[(n > 0) ? (size_t)n : 0] = '\0';
    ASSERT_TRUE(strstr(buf, "track_expired") != NULL, "client 1 receives broadcast");
    n = read(c2, buf, sizeof buf - 1);
    buf[(n > 0) ? (size_t)n : 0] = '\0';
    ASSERT_TRUE(strstr(buf, "track_expired") != NULL, "client 2 receives broadcast");

    close(c1);
    close(c2);
    server_close(&server);
}

static void test_config(void){
    korax_config_t cfg;

    /* 1. defaults produce the expected baseline */
    ASSERT_EQ_INT(config_defaults(&cfg), 0, "config_defaults succeeds");
    ASSERT_EQ_INT(cfg.expiry_seconds, 10, "default expiry_seconds is 10");
    ASSERT_EQ_INT(cfg.grace_seconds, 5, "default grace_seconds is 5");
    ASSERT_TRUE(cfg.max_speed_kmh == 150.0, "default max_speed_kmh is 150");
    ASSERT_EQ_INT(cfg.log_level, 2, "default log_level is 2");
    ASSERT_TRUE(strcmp(cfg.socket_path, KX_DEFAULT_SOCKET) == 0, "default socket path");

    /* 2. precedence: file overrides default, then CLI overrides file */
    const char *path = "/tmp/korax_test.conf";
    FILE *f = fopen(path, "w");
    ASSERT_TRUE(f != NULL, "temp config opens for write");
    fputs("[tracks]\nexpiry_seconds = 20\n", f);
    fclose(f);
    ASSERT_EQ_INT(config_load_file(&cfg, path), 0, "config_load_file succeeds");
    ASSERT_EQ_INT(cfg.expiry_seconds, 20, "file overrides default (20)");

    char *argv[] = { "koraxd", "--expiry", "30", NULL };
    optind = 1;                                  /* reset getopt scan state */
    ASSERT_EQ_INT(config_apply_cli(&cfg, 3, argv), 0, "config_apply_cli succeeds");
    ASSERT_EQ_INT(cfg.expiry_seconds, 30, "CLI overrides file (30)");

    /* 3. missing file keeps current values (not an error) */
    ASSERT_EQ_INT(config_load_file(&cfg, "/tmp/korax_does_not_exist.conf"), 0,
                  "missing config file -> ok, defaults kept");

    /* 4. bad input: an unknown key is reported as an error, not ignored */
    f = fopen(path, "w");
    fputs("[tracks]\nnonsense_key = 5\n", f);
    fclose(f);
    ASSERT_TRUE(config_load_file(&cfg, path) != 0, "unknown key -> error");

    remove(path);
}

static void test_logging(void){
    /* Level filtering: at threshold INFO, ERROR/WARN/INFO show, DEBUG is dropped. */
    log_set_level(LOG_INFO);
    ASSERT_TRUE(log_enabled(LOG_ERROR),  "ERROR enabled at INFO threshold");
    ASSERT_TRUE(log_enabled(LOG_WARN),   "WARN enabled at INFO threshold");
    ASSERT_TRUE(log_enabled(LOG_INFO),   "INFO enabled at INFO threshold");
    ASSERT_TRUE(!log_enabled(LOG_DEBUG), "DEBUG suppressed at INFO threshold");

    /* Lower the threshold to ERROR: only ERROR shows. */
    log_set_level(LOG_ERROR);
    ASSERT_TRUE(log_enabled(LOG_ERROR),  "ERROR enabled at ERROR threshold");
    ASSERT_TRUE(!log_enabled(LOG_WARN),  "WARN suppressed at ERROR threshold");
    ASSERT_TRUE(!log_enabled(LOG_DEBUG), "DEBUG suppressed at ERROR threshold");

    /* Bounded flood: many failures in one window produce at most one summary. */
    time_t t0 = 100000;
    int emitted = 0;
    for (int i = 0; i < 500; i++)
        emitted += log_decode_failure(t0);      /* all in the same instant */
    ASSERT_EQ_INT(emitted, 0, "flood within one window -> no output");

    /* Once the report interval has elapsed, exactly one summary is emitted. */
    ASSERT_EQ_INT(log_decode_failure(t0 + 10), 1, "after interval -> one summary line");

    /* Counter reset: the next failure starts a fresh window (no immediate flush). */
    ASSERT_EQ_INT(log_decode_failure(t0 + 10), 0, "post-flush -> counting again, no output");
}

static void test_pack_multi_drone(void){
    /* A Message Pack carrying two Basic ID sub-messages for two different
     * drones. The second drone lands on track index 1, which the old
     * `!= 0` check mistook for an error and truncated the pack (KAN-73). */
    uint8_t pack[3 + 2 * RID_MSG_SIZE];
    memset(pack, 0, sizeof pack);
    pack[0] = (uint8_t)((MESSAGETYPE_PACKED << 4) | 0x02);  /* pack, version 2   */
    pack[1] = RID_MSG_SIZE;                                 /* single msg size 25 */
    pack[2] = 2;                                            /* two sub-messages   */

    uint8_t *a = pack + 3;                                  /* drone A basic id   */
    a[0] = (uint8_t)((MESSAGETYPE_BASIC_ID << 4) | 0x02);
    a[1] = 0x12;                                            /* IDType=1, UAType=2 */
    memset(&a[2], 'A', 20);

    uint8_t *b = pack + 3 + RID_MSG_SIZE;                   /* drone B basic id   */
    b[0] = (uint8_t)((MESSAGETYPE_BASIC_ID << 4) | 0x02);
    b[1] = 0x12;
    memset(&b[2], 'B', 20);

    track_table_t table;
    track_table_init(&table);

    ASSERT_EQ_INT(decode_message_pack(pack, sizeof pack, &table, &MAGPIE_DEFAULT_THRESHOLDS), 0, "multi-drone pack decodes");
    ASSERT_EQ_INT(table.num_tracks, 2, "both sub-messages land -> 2 tracks (2nd not dropped)");
    ASSERT_TRUE(find_track(&table, "AAAAAAAAAAAAAAAAAAAA") >= 0, "drone A tracked");
    ASSERT_TRUE(find_track(&table, "BBBBBBBBBBBBBBBBBBBB") >= 0, "drone B tracked");
}

static void test_magpie_config_thresholds(void){
    track_t t; memset(&t, 0, sizeof t);
    t.msg_count_location = 2;
    t.prev_location.Latitude = 51.0000; t.location.Latitude = 51.0005;  /* ~55 m */
    t.prev_location_seen = 1000; t.last_location_seen = 1010;           /* over 10 s */

    magpie_thresholds_t loose  = { 5, 150.0 };   /* 150 km/h cap */
    magpie_thresholds_t strict = { 5, 5.0 };     /* 5 km/h cap   */

    ASSERT_TRUE((magpie_validate_jump(&t, &loose).rule  & KX_RULE_IMPLAUSIBLE_JUMP) == 0,
                "20 km/h move OK under 150 km/h cap");
    ASSERT_TRUE(magpie_validate_jump(&t, &strict).rule & KX_RULE_IMPLAUSIBLE_JUMP,
                "same move flagged under 5 km/h cap (config changes verdict)");
}

static void count_expired_cb(const track_t *track, void *user_data){
    (void)track;
    (*(int *)user_data)++;
}

static void test_expiry(void){
    track_table_t table;
    track_table_init(&table);

    int a = track_table_create(&table, "EXPIREDRONE0");
    int b = track_table_create(&table, "EXPIREDRONE1");
    int c = track_table_create(&table, "FRESHDRONE2");
    ASSERT_TRUE(a >= 0 && b >= 0 && c >= 0, "three tracks created");

    table.tracks[a].last_seen = 1000;   /* stale */
    table.tracks[b].last_seen = 1000;   /* stale */
    table.tracks[c].last_seen = 1030;   /* fresh (within 60s of now) */

    int expired = 0;
    int removed = track_table_expire(&table, 1061, 60, count_expired_cb, &expired);

    ASSERT_EQ_INT(removed, 2, "two stale tracks removed");
    ASSERT_EQ_INT(expired, 2, "expiry callback fired once per removed track");
    ASSERT_EQ_INT(table.num_tracks, 1, "one fresh track remains");
    ASSERT_TRUE(find_track(&table, "FRESHDRONE2") >= 0, "fresh track survived");
    ASSERT_EQ_INT(find_track(&table, "EXPIREDRONE0"), TRACK_NOT_FOUND, "stale track gone");

    /* boundary: exactly at the timeout is NOT removed (now - last_seen == expiry) */
    track_table_init(&table);
    int d = track_table_create(&table, "BOUNDARY");
    table.tracks[d].last_seen = 1000;
    ASSERT_EQ_INT(track_table_expire(&table, 1060, 60, NULL, NULL), 0, "exactly at timeout -> not removed");
}

static void test_find_by_mac(void){
    track_table_t table;
    track_table_init(&table);

    uint8_t mac_a[6]    = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t mac_b[6]    = {0x11, 0x22, 0x33, 0x44, 0x55, 0x67};  /* differs only in last byte */
    uint8_t mac_none[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    ASSERT_EQ_INT(find_track_by_mac(&table, mac_a), TRACK_NOT_FOUND, "empty table -> NOT_FOUND");

    int a = track_table_create(&table, "MACDRONE_A");
    int b = track_table_create(&table, "MACDRONE_B");
    memcpy(table.tracks[a].source_mac, mac_a, 6);
    memcpy(table.tracks[b].source_mac, mac_b, 6);

    ASSERT_EQ_INT(find_track_by_mac(&table, mac_a), a, "mac_a -> track a");
    ASSERT_EQ_INT(find_track_by_mac(&table, mac_b), b, "mac_b (last byte differs) -> track b");
    ASSERT_EQ_INT(find_track_by_mac(&table, mac_none), TRACK_NOT_FOUND, "unknown mac -> NOT_FOUND");
}

int main(void){
    printf("Running Raven decoder tests (real ODID fixtures)...\n\n");
    test_string_helpers();
    test_basic_id();
    test_location();
    test_auth();
    test_self_id();
    test_system();
    test_operator_id();
    test_error_paths();
    test_message_pack_valid();
    test_message_pack_errors();
    test_jump_distance();
    test_magpie_reserved_bits();
    test_output_json();
    test_server_feed();
    test_config();
    test_logging();
    test_pack_multi_drone();
    test_expiry();
    test_find_by_mac();
    test_previous_location_tracking();
    test_first_location_has_no_previous();
    test_previous_location_rolls_forward();
    test_magpie_required_missing();
    test_magpie_grace_window();
    test_magpie_location_range();
    test_magpie_basic_id();
    test_magpie_jump();
    test_magpie_validate_composed();
    test_magpie_config_thresholds();
    printf("\nTests run   : %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    if (tests_failed == 0) { printf("\nALL TESTS PASSED\n"); return 0; }
    printf("\nSOME TESTS FAILED\n");
    return 1;
}



