#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "transport_tcp_server.h"
#include "camera.h"
#include "hal.h"

#define TELEMTRY_LOG_MSG_MAX_SIZE 100

// Telemtry packet format
// Byte 0:   Packet type
// Byte 1-2: Packet size
// Byte 3-X: Payload 

typedef enum : uint8_t
{
    TELEMETRY_PACKET_STATUS = 0x01,
    TELEMETRY_PACKET_TAGS   = 0x02,
    TELEMETRY_PACKET_LOG    = 0x03,
    TELEMETRY_CMD_SET_DETECTION_PARAMS = 0x30,
} telemetry_packet_type_t;

typedef enum : uint8_t
{
    LOG_LEVEL_DEBUG   = 0,
    LOG_LEVEL_INFO    = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR   = 3,
} log_level_t;

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
}__attribute__((packed)) telemetry_status_t;
// -- TELEMTRY_START END -- //

typedef struct
{
    uint8_t  level;
    uint32_t timestamp;
    char     msg[TELEMTRY_LOG_MSG_MAX_SIZE + 1]; // Last byte is null
}__attribute__((packed)) telemetry_log_t;

typedef struct
{
    telemetry_packet_type_t type;
    uint16_t len;
    uint8_t* data;
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
        void init(Camera* camera);
        void update();
        bool sendLogMsg(const log_level_t level, const char* msg);

    private:
        TransportTCP_Server* tcpServer;
        Camera* camera;
        telemetry_status_t status;
        HAL_Queue<telemetry_log_t>* logQueue;
        telemetry_log_t log_tx;
        TelemetryPacketParser telemParser;

        void handleTelemetryCommand(const telemetry_rx_packet_t* pkt);
};

#endif