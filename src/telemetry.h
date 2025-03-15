#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "transport_tcp_server.h"
#include "camera.h"
#include "target_detector.h"
#include "mavcom.h"
#include "hal.h"

#define TELEMTRY_LOG_MSG_MAX_SIZE 100

// Telemtry packet format
// Byte 0:   Packet type
// Byte 1-2: Packet size
// Byte 3-X: Payload 

typedef enum : uint8_t
{
    TELEMETRY_PACKET_STATUS            = 0x01,
    TELEMETRY_PACKET_TAGS              = 0x02,
    TELEMETRY_PACKET_LOG               = 0x03,
    TELEMETRY_PACKET_PING              = 0x10,

    // Control commands
    TELEMETRY_CMD_SET_DETECTION_PARAMS = 0x30,
    TELEMETRY_CMD_ACTION               = 0x31
} telemetry_packet_type_t;

typedef enum : uint8_t
{
    LOG_LEVEL_DEBUG   = 0,
    LOG_LEVEL_INFO    = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR   = 3,
} log_level_t;

typedef enum : uint8_t
{
    LOG_GROUP_MAIN = 0,
    LOG_GROUP_AP   = 1,
} log_group_t;

// -- TELEMTRY_START START -- //
typedef struct
{
    uint32_t upTimeMs;
    uint8_t  cameraFps;
    uint32_t freeHeap;
    float quad_decimate;
    float quad_sigma;
    bool  refine_edges;
    float decode_sharpening;

    uint32_t ap_last_heartbeat;
    uint8_t  ap_mav_mode_flag;
    uint8_t  ap_mav_state;
    uint8_t  ap_flight_mode;
    float    ap_roll;
    float    ap_pitch;
    float    ap_yaw;
    float    ap_roll_speed;
    float    ap_pitch_speed;
    float    ap_yaw_speed;
    float    ap_rngfnd_dist_m;
    float    ap_bat_voltage;
    int8_t   ap_bat_perc;
    float ap_local_pos_x;
    float ap_local_pos_y;
    float ap_local_pos_z;
    float ap_local_pos_vx;
    float ap_local_pos_vy;
    float ap_local_pos_vz;
    float ap_relative_alt;
    float ap_alt;
    uint16_t ap_chan1_raw;
    uint16_t ap_chan2_raw;
    uint16_t ap_chan3_raw;
    uint16_t ap_chan4_raw;
    uint16_t ap_chan5_raw;
    uint16_t ap_chan6_raw;
    uint16_t ap_chan7_raw;
    uint16_t ap_chan8_raw;
    uint16_t ap_chan9_raw;
    uint16_t ap_chan10_raw;
    uint8_t  ap_rssi;
}__attribute__((packed)) telemetry_status_t;
// -- TELEMTRY_START END -- //

#define TELEMETRY_PACKET_MAX_PAYLOAD_SIZE 512
#define TELEMETRY_PACKET_HEADER_SIZE 5

typedef struct
{
    uint8_t  level;
    uint8_t  group;
    uint32_t timestamp;
    char     msg[TELEMTRY_LOG_MSG_MAX_SIZE + 1]; // Last byte is null
}__attribute__((packed)) telemetry_log_t;

typedef struct
{
    uint16_t magic;
    telemetry_packet_type_t type;
    uint16_t len;
    uint8_t payload[TELEMETRY_PACKET_MAX_PAYLOAD_SIZE];
}__attribute__((packed)) telemetry_packet_t;

typedef struct
{
    telemetry_packet_type_t type;
    uint8_t len;
    uint8_t data[255];
}__attribute__((packed)) telemetry_rx_packet_t;


class TelemetryPacketParser
{
    public:
        TelemetryPacketParser();
        bool parseByte(const uint8_t byte, telemetry_rx_packet_t* packet);
    private:
        typedef enum
        {
            PARSE_STATE_TYPE,
            PARSE_STATE_LEN,
            PARSE_STATE_DATA
        } parse_state_t;
        parse_state_t parseState;
        telemetry_rx_packet_t rxPacket;
        uint32_t parsedDataBytes;
        void resetParseState();
};

class Telemetry
{
    public:
        Telemetry(const int tcpPort);
        void init(Camera* camera, TargetDetector* targetDetector, MavCom* mavcom);
        void update();
        bool sendLogMsg(const log_level_t level, const log_group_t group, const char* msg);
        bool clientConnected();
        bool getClientIP(char ip[17]);

    private:
        TransportTCP_Server* tcpServer;
        Camera* camera;
        TargetDetector* targetDetector;
        MavCom* mavcom;
        telemetry_status_t status;
        HAL_Queue<telemetry_log_t>* logQueue;
        telemetry_log_t log_tx;
        TelemetryPacketParser telemParser;

        uint32_t lastTelemetryPing;

        void handleTelemetryCommand(const telemetry_rx_packet_t* pkt);
        void sendTelemetryPacket(const telemetry_packet_type_t packetType, const uint8_t* data, const uint32_t len);
};

#endif