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
    constexpr int packetSize = 1 + sizeof(telemetry_status_t);
    static uint8_t buf[packetSize];
    
    status.upTimeMs = millis();
    status.cameraFps = (uint8_t) camera->getFps();

    // Construct packet
    buf[0] = TELEMETRY_PACKET_STATUS;
    memcpy(&buf[1], &status, sizeof(telemetry_status_t));

    tcpServer->writeBytes(buf, packetSize);
    tcpServer->flushTX();
}