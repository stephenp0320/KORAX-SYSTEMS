#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"

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

    ASSERT_EQ_INT(decode_message_pack((uint8_t *)PKT_PACK, sizeof(PKT_PACK), &table), DECODE_OK, "pack decodes");
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
    ASSERT_EQ_INT(decode_message_pack(zero_count, sizeof(zero_count), &table), DECODE_ERR_RANGE, "pack count 0 -> RANGE");

    uint8_t big_count[3] = { 0xF2, 0x19, 0xFF };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(big_count, sizeof(big_count), &table), DECODE_ERR_RANGE, "pack count 255 -> RANGE");

    uint8_t bad_size[3] = { 0xF2, 0x18, 0x03 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(bad_size, sizeof(bad_size), &table), DECODE_ERR_RANGE, "pack bad single size -> RANGE");

    uint8_t truncated[10] = { 0xF2, 0x19, 0x02, 0x02, 0x12, 0x31 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(truncated, sizeof(truncated), &table), DECODE_ERR_TOO_SHORT, "pack truncated -> TOO_SHORT");

    uint8_t tiny[2] = { 0xF2, 0x19 };
    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack(tiny, sizeof(tiny), &table), DECODE_ERR_TOO_SHORT, "pack tiny header -> TOO_SHORT");

    track_table_init(&table);
    ASSERT_EQ_INT(decode_message_pack((uint8_t *)PKT_LOCATION, sizeof(PKT_LOCATION), &table), DECODE_ERR_BAD_TYPE, "non-pack -> BAD_TYPE");
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
    test_previous_location_tracking();
    test_first_location_has_no_previous();
    test_previous_location_rolls_forward();
    printf("\nTests run   : %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    if (tests_failed == 0) { printf("\nALL TESTS PASSED\n"); return 0; }
    printf("\nSOME TESTS FAILED\n");
    return 1;
}
