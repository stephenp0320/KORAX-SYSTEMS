#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "raven_decode.h"
#include "rook_track.h"
static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, message)                                      \
    do {                                                                     \
        tests_run++;                                                         \
        if (!(condition)) {                                                  \
            tests_failed++;                                                  \
            printf("FAIL: %s (line %d)\n", message, __LINE__);             \
        }                                                                    \
    } while (0)

#define ASSERT_EQ_INT(actual, expected, message)                             \
    do {                                                                     \
        tests_run++;                                                         \
        long long a_ = (long long)(actual);                                  \
        long long e_ = (long long)(expected);                                \
        if (a_ != e_) {                                                      \
            tests_failed++;                                                  \
            printf("FAIL: %s (line %d) expected %lld got %lld\n",           \
                   message, __LINE__, e_, a_);                               \
        }                                                                    \
    } while (0)

#define ASSERT_EQ_STR(actual, expected, message)                             \
    do {                                                                     \
        tests_run++;                                                         \
        const char *a_ = (actual);                                           \
        const char *e_ = (expected);                                         \
        if (strcmp(a_, e_) != 0) {                                           \
            tests_failed++;                                                  \
            printf("FAIL: %s (line %d) expected '%s' got '%s'\n",           \
                   message, __LINE__, e_, a_);                               \
        }                                                                    \
    } while (0)

#define ASSERT_NEAR(actual, expected, tolerance, message)                    \
    do {                                                                     \
        tests_run++;                                                         \
        double a_ = (double)(actual);                                        \
        double e_ = (double)(expected);                                      \
        double diff_ = a_ - e_;                                              \
        if (diff_ < 0) diff_ = -diff_;                                      \
        if (diff_ > (tolerance)) {                                           \
            tests_failed++;                                                  \
            printf("FAIL: %s (line %d) expected %.7f got %.7f\n",           \
                   message, __LINE__, e_, a_);                               \
        }                                                                    \
    } while (0)

static void test_string_helpers(void)
{
    ASSERT_EQ_STR(uatype_to_str(DRONE_ID_UATYPE_AEROPLANE), "UATYPE_AEROPLANE", "UA type aeroplane string");
    ASSERT_EQ_STR(uatype_to_str(DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR), "UATYPE_HELICOPTER_OR_MULTIROTOR", "UA type multirotor string");
    ASSERT_EQ_STR(uatype_to_str((DRONE_ID_uatype_t)99), "UATYPE_UNKNOWN", "UA type unknown string");

    ASSERT_EQ_STR(idtype_to_str(DRONE_ID_IDTYPE_SERIAL_NUMBER), "IDTYPE_SERIAL_NUMBER", "ID type serial string");
    ASSERT_EQ_STR(idtype_to_str(DRONE_ID_IDTYPE_CAA_REGISTRATION_ID), "IDTYPE_CAA_REGISTRATION_ID", "ID type CAA string");
    ASSERT_EQ_STR(idtype_to_str((DRONE_ID_idtype_t)99), "IDTYPE_UNKNOWN", "ID type unknown string");

    ASSERT_EQ_STR(msg_type_to_str(MESSAGETYPE_BASIC_ID), "MESSAGETYPE_BASIC_ID", "Message type basic string");
    ASSERT_EQ_STR(msg_type_to_str(MESSAGETYPE_LOCATION), "MESSAGETYPE_LOCATION", "Message type location string");
    ASSERT_EQ_STR(msg_type_to_str(MESSAGETYPE_AUTH), "MESSAGETYPE_AUTH", "Message type auth string");
    ASSERT_EQ_STR(msg_type_to_str(MESSAGETYPE_SYSTEM), "MESSAGETYPE_SYSTEM", "Message type system string");
    ASSERT_EQ_STR(msg_type_to_str((messagetype_t)99), "MESSAGETYPE_INVALID", "Message type invalid string");

    ASSERT_EQ_STR(status_to_str(DRONE_ID_STATUS_UNDECLARED), "STATUS_UNDECLARED", "Status undeclared string");
    ASSERT_EQ_STR(status_to_str(DRONE_ID_STATUS_AIRBORNE), "STATUS_AIRBORNE", "Status airborne string");
    ASSERT_EQ_STR(status_to_str((DRONE_ID_status_t)99), "UNKNOWN_STATUS", "Status unknown string");

    ASSERT_EQ_STR(auth_type_to_str(AUTH_UAS_ID_SIGNATURE), "AUTH_UAS_ID_SIGNATURE", "Auth UAS signature string");
    ASSERT_EQ_STR(auth_type_to_str(AUTH_NONE), "AUTH_NONE", "Auth none string");
    ASSERT_EQ_STR(auth_type_to_str((auth_type_t)99), "UNKNOWN_AUTH", "Auth unknown string");
}

static void test_basic_id_success(void)
{
    uint8_t packet[] = {
        0x00,
        0x21,
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X'
    };

    droneID_basicID_data basicID;
    memset(&basicID, 0, sizeof(basicID));

    ASSERT_EQ_INT(decode_basic_id(packet, sizeof(packet), &basicID), 0, "Basic ID decode should succeed");
    ASSERT_EQ_INT(basicID.UAType, DRONE_ID_UATYPE_AEROPLANE, "Basic ID UAType");
    ASSERT_EQ_INT(basicID.IDType, DRONE_ID_IDTYPE_CAA_REGISTRATION_ID, "Basic ID IDType");
    ASSERT_EQ_STR(basicID.UASID, "1581F5BJD248Q00A8XVX", "Basic ID UASID");
    ASSERT_EQ_INT(basicID.UASID[DRONE_ID_SIZE], '\0', "Basic ID null terminator");
}

static void test_basic_id_failures(void)
{
    uint8_t short_packet[] = {0x00, 0x21};
    uint8_t wrong_type_packet[] = {
        0x10,
        0x21,
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X'
    };
    droneID_basicID_data basicID;

    ASSERT_EQ_INT(decode_basic_id(NULL, 22, &basicID), DRONE_ID_ERR, "Basic ID null packet fails");
    ASSERT_EQ_INT(decode_basic_id(short_packet, sizeof(short_packet), &basicID), DRONE_ID_ERR, "Basic ID short packet fails");
    ASSERT_EQ_INT(decode_basic_id(wrong_type_packet, sizeof(wrong_type_packet), &basicID), DRONE_ID_ERR, "Basic ID wrong message type fails");
    ASSERT_EQ_INT(decode_basic_id(wrong_type_packet, sizeof(wrong_type_packet), NULL), DRONE_ID_ERR, "Basic ID null output fails");
}

static void test_location_success(void)
{
    uint8_t packet[] = {
        0x10,
        0x02,
        0x5A,
        0x14,
        0x82,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x08, 0x98,
        0x08, 0x98,
        0x3C,
        0x0B,
        0x06,
        0x06,
        0x04,
        0x02,
        0x05, 0xDC
    };

    DRONE_ID_Location_data location;
    memset(&location, 0, sizeof(location));

    ASSERT_EQ_INT(decode_location_vector(packet, sizeof(packet), &location), 0, "Location decode should succeed");
    ASSERT_EQ_INT(location.Status, DRONE_ID_STATUS_AIRBORNE, "Location status");
    ASSERT_NEAR(location.Direction, 90.0, 0.001, "Location direction");
    ASSERT_NEAR(location.SpeedHorizontal, 20.0, 0.001, "Location horizontal speed raw decode");
    ASSERT_NEAR(location.SpeedVertical, 130.0, 0.001, "Location vertical speed raw decode");
    ASSERT_NEAR(location.Latitude, 52.6731904, 0.0000001, "Location latitude");
    ASSERT_NEAR(location.Longitude, -8.6274432, 0.0000001, "Location longitude");
    ASSERT_NEAR(location.AltitudeBaro, 100.0, 0.001, "Location barometric altitude");
    ASSERT_NEAR(location.AltitudeGeo, 100.0, 0.001, "Location geodetic altitude");
    ASSERT_NEAR(location.Height, 12.0, 0.001, "Location height lower nibble");
    ASSERT_EQ_INT(location.HorizAccuracy, 11, "Location horizontal accuracy");
    ASSERT_EQ_INT(location.VertAccuracy, 6, "Location vertical accuracy");
    ASSERT_EQ_INT(location.BaroAccuracy, 6, "Location baro accuracy");
    ASSERT_EQ_INT(location.SpeedAccuracy, 4, "Location speed accuracy");
    ASSERT_EQ_INT(location.TSAccuracy, 2, "Location timestamp accuracy");
    ASSERT_NEAR(location.TimeStamp, 150.0, 0.001, "Location timestamp");
}

static void test_location_invalid_timestamp_and_failures(void)
{
    uint8_t packet[] = {
        0x10, 0x00, 0x5A, 0x14, 0x82,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x08, 0x98, 0x08, 0x98,
        0x3C, 0x0B, 0x06, 0x06, 0x04, 0x02,
        0xFF, 0xFF
    };
    uint8_t short_packet[] = {0x10, 0x00, 0x5A};
    DRONE_ID_Location_data location;

    ASSERT_EQ_INT(decode_location_vector(packet, sizeof(packet), &location), 0, "Location invalid timestamp packet decodes");
    ASSERT_NEAR(location.TimeStamp, -1.0, 0.001, "Location invalid timestamp becomes -1");
    ASSERT_EQ_INT(decode_location_vector(NULL, 25, &location), DRONE_LOCATION_ERR, "Location null packet fails");
    ASSERT_EQ_INT(decode_location_vector(short_packet, sizeof(short_packet), &location), DRONE_LOCATION_ERR, "Location short packet fails");
    ASSERT_EQ_INT(decode_location_vector(packet, sizeof(packet), NULL), DRONE_LOCATION_ERR, "Location null output fails");
}

static void test_auth_page_zero_success(void)
{
    uint8_t packet[] = {
        0x20,
        0x01,
        0x00,
        0x02,
        0x3F,
        0x00, 0x01, 0xE2, 0x40,
        0xA1, 0xB2, 0xC3, 0xD4,
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC
    };

    authentication_data auth;
    memset(&auth, 0, sizeof(auth));

    ASSERT_EQ_INT(decode_auth(packet, sizeof(packet), &auth), 0, "Auth page 0 decode should succeed");
    ASSERT_EQ_INT(auth.msg_type, MESSAGETYPE_AUTH, "Auth message type");
    ASSERT_EQ_INT(auth.auth_type, AUTH_UAS_ID_SIGNATURE, "Auth type");
    ASSERT_EQ_INT(auth.data_page, 0, "Auth page 0 data page");
    ASSERT_EQ_INT(auth.last_page_idx, 2, "Auth last page index");
    ASSERT_EQ_INT(auth.length, 63, "Auth total length");
    ASSERT_EQ_INT(auth.timestamp, 123456, "Auth timestamp");
    ASSERT_EQ_INT(auth.AuthData[0], 0xA1, "Auth page 0 first data byte");
    ASSERT_EQ_INT(auth.AuthData[15], 0xCC, "Auth page 0 last copied data byte");
}

static void test_auth_nonzero_page_success_and_failures(void)
{
    uint8_t packet[] = {
        0x20,
        0x01,
        0x01,
        0xDD, 0xEE, 0xFF, 0x10,
        0x20, 0x30, 0x40, 0x50,
        0x60, 0x70, 0x80, 0x90,
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0x01, 0x02
    };
    uint8_t short_packet[] = {0x20, 0x01, 0x00};
    authentication_data auth;
    memset(&auth, 0, sizeof(auth));

    ASSERT_EQ_INT(decode_auth(packet, sizeof(packet), &auth), 0, "Auth page 1 decode should succeed");
    ASSERT_EQ_INT(auth.msg_type, MESSAGETYPE_AUTH, "Auth page 1 message type");
    ASSERT_EQ_INT(auth.auth_type, AUTH_UAS_ID_SIGNATURE, "Auth page 1 type");
    ASSERT_EQ_INT(auth.data_page, 1, "Auth page 1 data page");
    ASSERT_EQ_INT(auth.AuthData[0], 0xDD, "Auth page 1 first data byte");
    ASSERT_EQ_INT(auth.AuthData[21], 0x02, "Auth page 1 last copied data byte");

    ASSERT_EQ_INT(decode_auth(NULL, 25, &auth), AUTH_ERR, "Auth null packet fails");
    ASSERT_EQ_INT(decode_auth(short_packet, sizeof(short_packet), &auth), AUTH_ERR, "Auth short packet fails");
    ASSERT_EQ_INT(decode_auth(packet, sizeof(packet), NULL), AUTH_ERR, "Auth null output fails");
}

static void test_self_id_success_and_failures(void)
{
    uint8_t packet[] = {
        0x30,
        0x01,
        'I','n','s','p','e','c','t','i','o','n',' ',
        'F','l','i','g','h','t',
        0,0,0,0,0,0
    };
    uint8_t short_packet[] = {0x30, 0x01};
    selfID_data selfID;
    memset(&selfID, 0, sizeof(selfID));

    ASSERT_EQ_INT(decode_selfID(packet, sizeof(packet), &selfID), 0, "Self ID decode should succeed");
    ASSERT_EQ_INT(selfID.msg_type, MESSAGETYPE_SELF_ID, "Self ID message type");
    ASSERT_EQ_INT(selfID.DescType, DESC_TYPE_EMERGENCY, "Self ID description type");
    ASSERT_EQ_STR(selfID.desc, "Inspection Flight", "Self ID description");
    ASSERT_EQ_INT(selfID.desc[DRONE_ID_STR_SIZE], '\0', "Self ID null terminator");

    ASSERT_EQ_INT(decode_selfID(NULL, 25, &selfID), DRONE_ID_ERR, "Self ID null packet fails");
    ASSERT_EQ_INT(decode_selfID(short_packet, sizeof(short_packet), &selfID), DRONE_ID_ERR, "Self ID short packet fails");
    ASSERT_EQ_INT(decode_selfID(packet, sizeof(packet), NULL), DRONE_ID_ERR, "Self ID null output fails");
}

static void test_system_success_and_failures(void)
{
    uint8_t packet[] = {
        0x40,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x02,
        0x64,
        0x08, 0x98,
        0x07, 0xF8,
        0x21,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00
    };
    uint8_t short_packet[] = {0x40, 0x1F};
    sys_data system;
    memset(&system, 0, sizeof(system));

    ASSERT_EQ_INT(decode_system_msg(packet, sizeof(packet), &system), 0, "System decode should succeed");
    ASSERT_EQ_INT(system.msg_type, MESSAGETYPE_SYSTEM, "System message type");
    ASSERT_EQ_INT(system.OperatorLocType, OP_LOCATION_TYPE_TAKEOFF, "System operator location type default");
    ASSERT_EQ_INT(system.ClassificationType, CLASSIFICATION_TYPE_UNDECLARED, "System classification type default");
    ASSERT_NEAR(system.OperatorLatitude, 52.6731904, 0.0000001, "System operator latitude");
    ASSERT_NEAR(system.OperatorLongitude, -8.6274432, 0.0000001, "System operator longitude");
    ASSERT_EQ_INT(system.AreaCount, 2, "System area count");
    ASSERT_EQ_INT(system.AreaRadius, 100, "System area radius");
    ASSERT_NEAR(system.AreaCeiling, 100.0, 0.001, "System area ceiling");
    ASSERT_NEAR(system.AreaFloor, 20.0, 0.001, "System area floor");
    ASSERT_EQ_INT(system.CategoryEU, 0x21, "System category EU raw value");
    ASSERT_EQ_INT(system.ClassEU, CLASS_EU_UNDECLARED, "System class EU default");
    ASSERT_NEAR(system.OperatorAltitudeGeo, 0.0, 0.001, "System operator altitude default");
    ASSERT_EQ_INT(system.Timestamp, 0, "System timestamp default");

    ASSERT_EQ_INT(decode_system_msg(NULL, 25, &system), DRONE_ID_ERR, "System null packet fails");
    ASSERT_EQ_INT(decode_system_msg(short_packet, sizeof(short_packet), &system), DRONE_ID_ERR, "System short packet fails");
    ASSERT_EQ_INT(decode_system_msg(packet, sizeof(packet), NULL), DRONE_ID_ERR, "System null output fails");
}

static void test_operator_id_success_and_failures(void)
{
    uint8_t packet[] = {
        0x50,
        0x00,
        'I','E','-','O','P','-','1','2','3','4',
        '5','6','7','8','9','A','B','C','D','E',
        0x00, 0x00, 0x00
    };
    uint8_t short_packet[] = {0x50, 0x00};
    OperatorID_data op;
    memset(&op, 0, sizeof(op));

    ASSERT_EQ_INT(decode_operator_id(packet, sizeof(packet), &op), 0, "Operator ID decode should succeed");
    ASSERT_EQ_INT(op.msg_type, MESSAGETYPE_OPERATOR_ID, "Operator ID message type");
    ASSERT_EQ_INT(op.OperatorIdType, ODID_OPERATOR_ID, "Operator ID type");
    ASSERT_EQ_STR(op.OperatorId, "IE-OP-123456789ABCDE", "Operator ID string");
    ASSERT_EQ_INT(op.OperatorId[OPERATOR_ID_SIZE], '\0', "Operator ID null terminator");

    ASSERT_EQ_INT(decode_operator_id(NULL, 25, &op), OPP_ID_ERR, "Operator ID null packet fails");
    ASSERT_EQ_INT(decode_operator_id(short_packet, sizeof(short_packet), &op), OPP_ID_ERR, "Operator ID short packet fails");
    ASSERT_EQ_INT(decode_operator_id(packet, sizeof(packet), NULL), OPP_ID_ERR, "Operator ID null output fails");
}

static void test_message_pack(void){
    uint8_t message_pack[1 + (3 * RID_MSG_SIZE)] = {
    0xF3,
    // Message 1: Basic ID
    0x00, 0x21,
    '1','5','8','1','F','5','B','J','D','2',
    '4','8','Q','0','0','A','8','X','V','X',
    0x00, 0x00, 0x00,

    // Message 2: Location
    0x10, 0x00, 0x5A, 0x14, 0x82,
    0x1F, 0x65, 0x4A, 0x80,
    0xFA, 0xDB, 0x8E, 0x80,
    0x08, 0x98,
    0x08, 0x98,
    0x3C,
    0x0B, 0x06, 0x06, 0x04, 0x02,
    0x05, 0xDC,

    // Message 3: System
    0x40,
    0x1F, 0x65, 0x4A, 0x80,
    0xFA, 0xDB, 0x8E, 0x80,
    0x02,
    0x64,
    0x08, 0x98,
    0x07, 0xF8,
    0x21,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00
};

    track_table_t table;
    track_table_init(&table);
    int result = decode_message_pack(message_pack, sizeof(message_pack), &table);
    ASSERT_TRUE(result == 0, "Message Pack decode failed");
    ASSERT_TRUE(table.num_tracks == 1, "Message Pack should create one track");
    ASSERT_TRUE(table.tracks[0].has_basic_id == 1, "Basic ID missing");
    ASSERT_TRUE(table.tracks[0].has_location == 1, "Location missing");
    ASSERT_TRUE(table.tracks[0].has_system == 1, "System missing");
}

static int test_message_pack_valid_2_messages(void)
{
    track_table_t table;
    track_table_init(&table);

    uint8_t pack[1 + (2 * RID_MSG_SIZE)] = {
        0xF2,

        // Basic ID
        0x00, 0x21,
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X',
        0x00, 0x00, 0x00,

        // Location
        0x10, 0x00, 0x5A, 0x14, 0x82,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x08, 0x98, 0x08, 0x98,
        0x3C, 0x0B, 0x06, 0x06, 0x04, 0x02,
        0x05, 0xDC
    };

    ASSERT_TRUE(decode_message_pack(pack, sizeof(pack), &table) == 0,
                "Message Pack 2-message decode failed");

    ASSERT_TRUE(table.num_tracks == 1, "Message Pack should create one track");
    ASSERT_TRUE(table.tracks[0].has_basic_id == 1, "Message Pack missing Basic ID");
    ASSERT_TRUE(table.tracks[0].has_location == 1, "Message Pack missing Location");

    return 0;
}

static int test_message_pack_valid_3_messages(void)
{
    track_table_t table;
    track_table_init(&table);

    uint8_t pack[1 + (3 * RID_MSG_SIZE)] = {
        0xF3,

        // Basic ID
        0x00, 0x21,
        '1','5','8','1','F','5','B','J','D','2',
        '4','8','Q','0','0','A','8','X','V','X',
        0x00, 0x00, 0x00,

        // Location
        0x10, 0x00, 0x5A, 0x14, 0x82,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x08, 0x98, 0x08, 0x98,
        0x3C, 0x0B, 0x06, 0x06, 0x04, 0x02,
        0x05, 0xDC,

        // System
        0x40,
        0x1F, 0x65, 0x4A, 0x80,
        0xFA, 0xDB, 0x8E, 0x80,
        0x02, 0x64,
        0x08, 0x98,
        0x07, 0xF8,
        0x21,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00
    };

    ASSERT_TRUE(decode_message_pack(pack, sizeof(pack), &table) == 0,
                "Message Pack 3-message decode failed");

    ASSERT_TRUE(table.num_tracks == 1, "Message Pack should create one track");
    ASSERT_TRUE(table.tracks[0].has_basic_id == 1, "Basic ID missing");
    ASSERT_TRUE(table.tracks[0].has_location == 1, "Location missing");
    ASSERT_TRUE(table.tracks[0].has_system == 1, "System missing");

    return 0;
}

static int test_message_pack_truncated(void)
{
    track_table_t table;
    track_table_init(&table);

    uint8_t pack[10] = {
        0xF2,
        0x00, 0x21, '1','5','8','1','F','5','B'
    };

    ASSERT_TRUE(decode_message_pack(pack, sizeof(pack), &table) != 0,
                "Truncated Message Pack should fail");

    return 0;
}

static int test_message_pack_zero_messages(void)
{
    track_table_t table;
    track_table_init(&table);

    uint8_t pack[] = {
        0xF0
    };

    ASSERT_TRUE(decode_message_pack(pack, sizeof(pack), &table) != 0,
                "Zero-message Message Pack should fail");

    return 0;
}

static int test_message_pack_nested_pack(void)
{
    track_table_t table;
    track_table_init(&table);

    uint8_t pack[1 + RID_MSG_SIZE] = {
        0xF1,

        // Contained message has type 0x0F = Message Pack
        0xF0,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    ASSERT_TRUE(decode_message_pack(pack, sizeof(pack), &table) != 0,
                "Nested Message Pack should fail");

    return 0;
}

int main(void)
{
    printf("Running full decoder tests...\n\n");

    test_string_helpers();
    test_basic_id_success();
    test_basic_id_failures();
    test_location_success();
    test_location_invalid_timestamp_and_failures();
    test_auth_page_zero_success();
    test_auth_nonzero_page_success_and_failures();
    test_self_id_success_and_failures();
    test_system_success_and_failures();
    test_operator_id_success_and_failures();
    test_message_pack();
    test_message_pack_valid_2_messages();
    test_message_pack_valid_3_messages();
    test_message_pack_truncated();
    test_message_pack_zero_messages();
    test_message_pack_nested_pack();

    printf("\nTests run   : %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }

    printf("\nSOME TESTS FAILED\n");
    return 1;
}
