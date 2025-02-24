#include "telemetry.h"
#include "hal.h"


Telemetry::Telemetry(const int tcpPort)
{
    tcpServer = new TCP_SERVER_CLASS(tcpPort);
    logQueue = new HAL_QUEUE_CLASS(telemetry_log_t, 10);
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
    if (!tcpServer->clientConnected())
    {
        return;
    }

    const int header_size = 3;
    uint32_t packetSize = header_size + sizeof(telemetry_status_t);
    static uint8_t buf[256];
    
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

    tcpServer->writeBytes(buf, header_size+tx->len);
    tcpServer->flushTX();

    // Empty log queue
    while (!logQueue->isEmpty())
    {
        telemetry_log_t log;
        logQueue->pop(log, 0);

        // Byte 0 is packet type
        tx->type = TELEMETRY_PACKET_LOG;
        // Byte 1-2 is packet length
        tx->len = sizeof(telemetry_log_t);

        memcpy(&buf[header_size], (uint8_t*) &log, tx->len);

        tcpServer->writeBytes(buf, header_size + tx->len);
        tcpServer->flushTX();
    }

}

bool Telemetry::sendLogMsg(const log_level_t level, const char* msg)
{
    // Create new log block and put to queue
    telemetry_log_t log;
    log.level = level;
    log.timestamp = millis();
    strncpy(log.msg, msg, TELEMTRY_LOG_MSG_MAX_SIZE);
    log.msg[TELEMTRY_LOG_MSG_MAX_SIZE] = 0; // Null termination

    return logQueue->push(log, 0);
}
