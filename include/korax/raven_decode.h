#include <stdint.h>
#ifndef DECODER_H
#define DECODER_H
#define MSG_SUCCESS 0
#define BLE_LEGACY_ADV_MAX_SIZE   31
#define RID_MESSAGE_SIZE          25
#define AUTH_PACK_SIZE 22
#define DRONE_ID_PACK_SIZE 22
#define SELFID_PACK_SIZE 25
#define SYSMSG_PACK_SIZE 25
#define OPID_PACK_SIZE 25
#define DJI_MAX_SIZE 20
#define MIN_DIR         0       // Minimum direction
#define MAX_DIR         360     // Maximum direction
#define INV_DIR         361     // Invalid direction
#define MIN_SPEED_H     0       // Minimum speed horizontal
#define MAX_SPEED_H     254.25f // Maximum speed horizontal
#define INV_SPEED_H     255     // Invalid speed horizontal
#define MIN_SPEED_V     (-62)   // Minimum speed vertical
#define MAX_SPEED_V     62      // Maximum speed vertical
#define INV_SPEED_V     63      // Invalid speed vertical
#define MIN_LAT         (-90)   // Minimum latitude
#define MAX_LAT         90      // Maximum latitude
#define MIN_LON         (-180)  // Minimum longitude
#define MAX_LON         180     // Maximum longitude
#define MIN_ALT         (-1000) // Minimum altitude
#define MAX_ALT         31767.5f// Maximum altitude
#define INV_ALT         MIN_ALT // Invalid altitude
#define MIN_ALT_BARO    -1000
#define MAX_ALT_BARO    31767.5
#define MAX_TIMESTAMP   (60 * 60) // 1 hour in seconds
#define INV_TIMESTAMP   0xFFFF
#define MAX_AREA_RADIUS 2550
#define DRONE_ID_SIZE 20
#define DRONE_ID_STR_SIZE 23
#define OPERATOR_ID_SIZE 20
#define RID_MSG_SIZE 25
#define MAX_PACK_MSG 100

typedef enum {
    DECODE_OK = 0,
    DECODE_ERR_NULL,
    DECODE_ERR_TOO_SHORT,      // fewer bytes than the message needs
    DECODE_ERR_BAD_TYPE,       // message-type header not recognised
    DECODE_ERR_BAD_VERSION,    // protocol version this build doesn't handle
    DECODE_ERR_RESERVED_BITS,  // reserved fields set - decoded, but flagged
    DECODE_ERR_RANGE,          // a field value outside its spec-defined range
} decode_result_t;

typedef enum DRONE_ID_uatype {
    DRONE_ID_UATYPE_NONE = 0,
    DRONE_ID_UATYPE_AEROPLANE = 1,
    DRONE_ID_UATYPE_HELICOPTER_OR_MULTIROTOR = 2,
    DRONE_ID_UATYPE_GYROPLANE = 3,
    DRONE_ID_UATYPE_HYBRID_LIFT = 4,
    DRONE_ID_UATYPE_ORNITHOPTER = 5,
    DRONE_ID_UATYPE_GLIDER = 6,
    DRONE_ID_UATYPE_KITE = 7,
    DRONE_ID_UATYPE_FREE_BALLOON = 8,
    DRONE_ID_UATYPE_CAPTIVE_BALLOON = 9,
    DRONE_ID_UATYPE_AIRSHIP = 10,
    DRONE_ID_UATYPE_FREE_FALL_PARACHUTE = 11,
    DRONE_ID_UATYPE_ROCKET = 12,
    DRONE_ID_UATYPE_TETHERED_POWERED_AIRCRAFT = 13,
    DRONE_ID_UATYPE_GROUND_OBSTACLE = 14,
    DRONE_ID_UATYPE_OTHER = 15,
} DRONE_ID_uatype_t;

typedef enum DRONE_ID_idtype {
    DRONE_ID_IDTYPE_NONE = 0,
    DRONE_ID_IDTYPE_SERIAL_NUMBER = 1,
    DRONE_ID_IDTYPE_CAA_REGISTRATION_ID = 2, // Civil Aviation Authority
    DRONE_ID_IDTYPE_UTM_ASSIGNED_UUID = 3,   // UAS (Unmanned Aircraft System) Traffic Management
    DRONE_ID_IDTYPE_SPECIFIC_SESSION_ID = 4, // The exact id type is specified by the first byte of UASID and these type
                                         // values are managed by ICAO. 0 is reserved. 1 - 224 are managed by ICAO.
                                         // 225 - 255 are available for private experimental usage only
} DRONE_ID_idtype_t;

typedef struct droneID_basicID_data {
	DRONE_ID_uatype_t UAType;
	DRONE_ID_idtype_t IDType;
	char UASID[DRONE_ID_SIZE+1];
} droneID_basicID_data;

typedef enum DRONE_ID_status {
    DRONE_ID_STATUS_UNDECLARED = 0,
    DRONE_ID_STATUS_GROUND = 1,
    DRONE_ID_STATUS_AIRBORNE = 2,
    DRONE_ID_STATUS_EMERGENCY = 3,
    DRONE_ID_STATUS_REMOTE_ID_SYSTEM_FAILURE = 4,
} DRONE_ID_status_t;

typedef enum DRONE_ID_Height_reference {
    DRONE_ID_HEIGHT_REF_OVER_TAKEOFF = 0,
    DRONE_ID_HEIGHT_REF_OVER_GROUND = 1,
} DRONE_ID_Height_reference_t;

typedef enum DRONE_ID_Horizontal_accuracy {
    DRONE_ID_HOR_ACC_UNKNOWN = 0,
    DRONE_ID_HOR_ACC_10NM = 1,      // Nautical Miles. 18.52 km
    DRONE_ID_HOR_ACC_4NM = 2,       // 7.408 km
    DRONE_ID_HOR_ACC_2NM = 3,       // 3.704 km
    DRONE_ID_HOR_ACC_1NM = 4,       // 1.852 km
    DRONE_ID_HOR_ACC_0_5NM = 5,     // 926 m
    DRONE_ID_HOR_ACC_0_3NM = 6,     // 555.6 m
    DRONE_ID_HOR_ACC_0_1NM = 7,     // 185.2 m
    DRONE_ID_HOR_ACC_0_05NM = 8,    // 92.6 m
    DRONE_ID_HOR_ACC_30_METER = 9,
    DRONE_ID_HOR_ACC_10_METER = 10,
    DRONE_ID_HOR_ACC_3_METER = 11,
    DRONE_ID_HOR_ACC_1_METER = 12,
    // 13 to 15 reserved
} DRONE_ID_Horizontal_accuracy_t;

typedef enum DRONE_ID_Vertical_accuracy {
    DRONE_ID_VER_ACC_UNKNOWN = 0,
    DRONE_ID_VER_ACC_150_METER = 1,
    DRONE_ID_VER_ACC_45_METER = 2,
    DRONE_ID_VER_ACC_25_METER = 3,
    DRONE_ID_VER_ACC_10_METER = 4,
    DRONE_ID_VER_ACC_3_METER = 5,
    DRONE_ID_VER_ACC_1_METER = 6,
} DRONE_ID_Vertical_accuracy_t;

typedef enum DRONE_ID_Speed_accuracy {
    DRONE_ID_SPEED_ACC_UNKNOWN = 0,
    DRONE_ID_SPEED_ACC_10_METERS_PER_SECOND = 1,
    DRONE_ID_SPEED_ACC_3_METERS_PER_SECOND = 2,
    DRONE_ID_SPEED_ACC_1_METERS_PER_SECOND = 3,
    DRONE_ID_SPEED_ACC_0_3_METERS_PER_SECOND = 4,
} DRONE_ID_Speed_accuracy_t;

typedef enum DRONE_ID_Timestamp_accuracy {
    DRONE_ID_TIME_ACC_UNKNOWN = 0,
    DRONE_ID_TIME_ACC_0_1_SECOND = 1,
    DRONE_ID_TIME_ACC_0_2_SECOND = 2,
    DRONE_ID_TIME_ACC_0_3_SECOND = 3,
    DRONE_ID_TIME_ACC_0_4_SECOND = 4,
    DRONE_ID_TIME_ACC_0_5_SECOND = 5,
    DRONE_ID_TIME_ACC_0_6_SECOND = 6,
    DRONE_ID_TIME_ACC_0_7_SECOND = 7,
    DRONE_ID_TIME_ACC_0_8_SECOND = 8,
    DRONE_ID_TIME_ACC_0_9_SECOND = 9,
    DRONE_ID_TIME_ACC_1_0_SECOND = 10,
    DRONE_ID_TIME_ACC_1_1_SECOND = 11,
    DRONE_ID_TIME_ACC_1_2_SECOND = 12,
    DRONE_ID_TIME_ACC_1_3_SECOND = 13,
    DRONE_ID_TIME_ACC_1_4_SECOND = 14,
    DRONE_ID_TIME_ACC_1_5_SECOND = 15,
} DRONE_ID_Timestamp_accuracy_t;

typedef struct DRONE_ID_Location_data {
    DRONE_ID_status_t Status;
    float Direction;          // Degrees. 0 <= x < 360. Route course based on true North. Invalid, No Value, or Unknown: 361deg
    float SpeedHorizontal;    // m/s. Positive only. Invalid, No Value, or Unknown: 255m/s. If speed is >= 254.25 m/s: 254.25m/s
    float SpeedVertical;      // m/s. Invalid, No Value, or Unknown: 63m/s. If speed is >= 62m/s: 62m/s
    double Latitude;          // Invalid, No Value, or Unknown: 0 deg (both Lat/Lon)
    double Longitude;         // Invalid, No Value, or Unknown: 0 deg (both Lat/Lon)
    float AltitudeBaro;       // meter (Ref 29.92 inHg, 1013.24 mb). Invalid, No Value, or Unknown: -1000m
    float AltitudeGeo;        // meter (WGS84-HAE). Invalid, No Value, or Unknown: -1000m
    DRONE_ID_Height_reference_t HeightType;
    float Height;             // meter. Invalid, No Value, or Unknown: -1000m
    DRONE_ID_Horizontal_accuracy_t HorizAccuracy;
    DRONE_ID_Vertical_accuracy_t VertAccuracy;
    DRONE_ID_Vertical_accuracy_t BaroAccuracy;
    DRONE_ID_Speed_accuracy_t SpeedAccuracy;
    DRONE_ID_Timestamp_accuracy_t TSAccuracy;
    float TimeStamp;          // seconds after the full hour relative to UTC. Invalid, No Value, or Unknown: 0xFFFF
} DRONE_ID_Location_data;

typedef enum messagetype {
    MESSAGETYPE_BASIC_ID = 0,
    MESSAGETYPE_LOCATION = 1,
    MESSAGETYPE_AUTH = 2,
    MESSAGETYPE_SELF_ID = 3,
    MESSAGETYPE_SYSTEM = 4,
    MESSAGETYPE_OPERATOR_ID = 5,
    MESSAGETYPE_PACKED = 0xF,
    MESSAGETYPE_INVALID = 0xFF,
} messagetype_t;


typedef enum authentication_type {
    AUTH_NONE = 0,
    AUTH_UAS_ID_SIGNATURE = 1,
    AUTH_OPERATOR_ID_SIGNATURE = 2,
    AUTH_MSG_SET_SIGNATURE = 3,
    AUTH_NET_REMOTE_ID = 4,
    AUTH_SPECIFIC_AUTH = 5,
} auth_type_t;

typedef struct authentication_data {
    uint8_t msg_type;
    uint8_t data_page;
    auth_type_t auth_type;
    uint8_t last_page_idx;
    uint8_t length;
    uint32_t timestamp;
    uint8_t AuthData[23+1];
} authentication_data;

typedef enum desctype {
    DESC_TYPE_TEXT = 0,
    DESC_TYPE_EMERGENCY = 1,       // is == EMERGENCY
    DESC_TYPE_EXTENDED_STATUS = 2, // not != EMERGENCY
} desctype_t;

typedef struct selfID_data {
    uint8_t msg_type;
    desctype_t DescType;
    char desc[DRONE_ID_STR_SIZE + 1];
} selfID_data;

typedef enum operator_location_type {
    OP_LOCATION_TYPE_TAKEOFF = 0,   // Takeoff location and altitude
    OP_LOCATION_TYPE_LIVE_GNSS = 1, // Dynamic/Live location and altitude
    OP_LOCATION_TYPE_FIXED = 2,     // Fixed location and altitude
} operator_location_type_t;

typedef enum classification_type {
    CLASSIFICATION_TYPE_UNDECLARED = 0,
    CLASSIFICATION_TYPE_EU = 1, // European Union
} classification_type_t;

typedef enum category_EU {
    CATEGORY_EU_UNDECLARED = 0,
    CATEGORY_EU_OPEN = 1,
    CATEGORY_EU_SPECIFIC = 2,
    CATEGORY_EU_CERTIFIED = 3,
} category_EU_t;

typedef enum class_EU {
    CLASS_EU_UNDECLARED = 0,
    CLASS_EU_CLASS_0 = 1,
    CLASS_EU_CLASS_1 = 2,
    CLASS_EU_CLASS_2 = 3,
    CLASS_EU_CLASS_3 = 4,
    CLASS_EU_CLASS_4 = 5,
    CLASS_EU_CLASS_5 = 6,
    CLASS_EU_CLASS_6 = 7,
    // 8 to 15 reserved
} class_EU_t;

typedef struct sys_data {
    uint8_t msg_type;
    operator_location_type_t OperatorLocType;
    classification_type_t ClassificationType;
    uint16_t AreaCount;
    double OperatorLatitude;
    double OperatorLongitude;
    uint16_t AreaRadius;
    float AreaCeiling;
    float AreaFloor;
    category_EU_t CategoryEU;
    class_EU_t ClassEU;
    float OperatorAltitudeGeo;
    uint32_t Timestamp;
} sys_data;

typedef enum operatorIdType {
    ODID_OPERATOR_ID = 0,
} operatorIdType_t;

typedef struct OperatorID_data {
    uint8_t msg_type;
    operatorIdType_t OperatorIdType;
    char OperatorId[OPERATOR_ID_SIZE+1];
} OperatorID_data;

typedef struct decoded_message {
    messagetype_t type;
    union {
        droneID_basicID_data basic_id;
        DRONE_ID_Location_data location;
        authentication_data auth;
        selfID_data self_id;
        sys_data system;
        OperatorID_data operator_id;
    } data;

} decoded_message_t;

typedef struct {
    uint8_t num_messages;
    uint8_t raw_messages[MAX_PACK_MSG][RID_MSG_SIZE];
} message_pack_t;

static inline uint16_t rd_u16le(const uint8_t *p){
    return (uint16_t)(p[0] | (p[1] << 8));
}

static inline int32_t rd_i32le(const uint8_t *p) {
    uint32_t v = (uint32_t)p[0]
               | ((uint32_t)p[1] << 8)
               | ((uint32_t)p[2] << 16)
               | ((uint32_t)p[3] << 24);
    return (int32_t)v;
}

static inline uint32_t rd_u32le(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

int decode_basic_id(const uint8_t *packet, int packet_len, droneID_basicID_data *basicID);
const char *uatype_to_str(DRONE_ID_uatype_t uatype);
const char *idtype_to_str(DRONE_ID_idtype_t idtype);
const char *msg_type_to_str(messagetype_t msg_type);
const char *auth_type_to_str(auth_type_t msg_type);
const char *status_to_str(DRONE_ID_status_t status_type);
int decode_location_vector(const uint8_t *packet, int packet_len, DRONE_ID_Location_data *location_data);
int decode_auth(const uint8_t *packet, int packet_len, authentication_data *auth_data);
int decode_selfID(const uint8_t *packet, int packet_len, selfID_data *selfID);
int decode_system_msg(const uint8_t *packet, int packet_len, sys_data *system_msg);
int decode_operator_id(const uint8_t *packet, int packet_len, OperatorID_data *op_id);
int decode_message(const uint8_t *packet, int packet_len, decoded_message_t *out);
#endif

