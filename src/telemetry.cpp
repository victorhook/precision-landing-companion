#include "telemetry.h"
#include "hal.h"


Telemetry::Telemetry(const int tcpPort)
{
    tcpServer = new TCP_SERVER_CLASS(tcpPort);
}

void Telemetry::init(Camera* camera)
{
    tcpServer->init();
    this->camera = camera;
    status.upTimeMs = millis();
    status.cameraFps = 0;
}

void Telemetry::update()
{
    const int header_size = 3;
    constexpr int packetSize = header_size + sizeof(telemetry_status_t);
    static uint8_t buf[packetSize];
    
    status.upTimeMs = millis();
    status.cameraFps = (uint8_t) camera->getFps();
    status.freeHeap = ESP.getFreeHeap();

    // Construct packet
    telemetry_packet_t* tx = (telemetry_packet_t*) buf;
    // Byte 0 is packet type
    tx->type = TELEMETRY_PACKET_STATUS;
    // Byte 1-2 is packet length
    tx->len = sizeof(telemetry_status_t);

    memcpy(&buf[header_size], &status, tx->len);

    tcpServer->writeBytes(buf, packetSize);
    tcpServer->flushTX();
}