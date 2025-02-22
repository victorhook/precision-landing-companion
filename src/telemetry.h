#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "transport_tcp_server.h"
#include "camera.h"

// Telemtry packet format
// Byte 0:   Packet type
// Byte 1:   Packet size
// Byte 2-X: Payload 

// -- TELEMTRY_START START -- //
typedef struct
{
    uint32_t upTimeMs;
    uint8_t  cameraFps;
    uint32_t freeHeap;
}__attribute__((packed)) telemetry_status_t;
// -- TELEMTRY_START END -- //

typedef enum : uint8_t
{
    TELEMETRY_PACKET_STATUS        = 0x01,
    TELEMETRY_PACKET_MARKERS_FOUND = 0x02,
} telemetry_packet_type_t;

typedef struct
{
    telemetry_packet_type_t type;
    uint16_t len;
    uint8_t* data;
}__attribute__((packed)) telemetry_packet_t;


class Telemetry
{
    public:
        Telemetry(const int tcpPort);
        void init(Camera* camera);
        void update();

    private:
        TransportTCP_Server* tcpServer;
        Camera* camera;
        telemetry_status_t status;
};

#endif