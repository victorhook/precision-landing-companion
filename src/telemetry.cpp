#include "telemetry.h"
#include "hal.h"

#include "log.h"



Telemetry::Telemetry(const int tcpPort)
{
    tcpServer = new TCP_SERVER_CLASS(tcpPort);
    logQueue = new HAL_QUEUE_CLASS(telemetry_log_t, 10);
}

void Telemetry::init(Camera* camera, TargetDetector* targetDetector)
{
    this->camera = camera;
    this->targetDetector = targetDetector;
    tcpServer->init();
    status.upTimeMs = millis();
    status.cameraFps = 0;
}

void Telemetry::update()
{
    if (!tcpServer->clientConnected())
    {
        return;
    }

    static uint8_t buf[256];
    telemetry_rx_packet_t packet;

    // Read potential incoming packets
    int bytesRead = tcpServer->readBytes(buf, 100, 0);
    for (int i = 0; i < bytesRead; i++)
    {
        if (telemParser.parseByte(buf[i], &packet))
        {
            info("RX PKT: %d (len=%d)\n", packet.type, packet.len);
            handleTelemetryCommand(&packet);
        }
    }

    // Send outgoing data

    const int header_size = 3;
    uint32_t packetSize = header_size + sizeof(telemetry_status_t);
    
    status.upTimeMs = millis();
    status.cameraFps = (uint8_t) camera->getFps();
    status.freeHeap = ESP.getFreeHeap();
    tag_detection_params_t detection_params = targetDetector->getTagDetectionParams();
    status.quad_decimate = detection_params.quad_decimate;
    status.quad_sigma = detection_params.quad_sigma;
    status.refine_edges = detection_params.refine_edges;
    status.decode_sharpening = detection_params.decode_sharpening;

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

    // Check tag detections
    static tag_position tags[10];
    uint8_t tags_detected = targetDetector->getTagsDetected(tags);
    tx->type = TELEMETRY_PACKET_TAGS;
    tx->len = 1 + (tags_detected * sizeof(telemetry_log_t));
    // First payload byte: nbr of tags, the remainder is tag data
    buf[header_size] = tags_detected;
    memcpy(&buf[header_size + 1], &tags, tx->len);
    tcpServer->writeBytes(buf, header_size + tx->len);
    tcpServer->flushTX();
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

void Telemetry::handleTelemetryCommand(const telemetry_rx_packet_t* pkt)
{
    tag_detection_params_t* detection_params;

    switch (pkt->type)
    {
        case TELEMETRY_CMD_SET_DETECTION_PARAMS:
            detection_params = (tag_detection_params_t*) pkt->data;
            targetDetector->setTagDetectionParams(detection_params);
            break;
        default:
            warning("Unkown command type: %02x\n", pkt->type);
    }
}

bool Telemetry::clientConnected()
{
    return tcpServer->clientConnected();
}

bool Telemetry::getClientIP(char ip[17])
{
    return tcpServer->getClientIp(ip);
}

// -- Telemetry parser -- //
TelemetryPacketParser::TelemetryPacketParser()
{
    resetParseState();
}

bool TelemetryPacketParser::parseByte(const uint8_t byte, telemetry_rx_packet_t* packet)
{
    bool foundPacket = false;
    //printf("%02x\n", byte);
    switch (parseState)
    {
        case PARSE_STATE_TYPE:
            rxPacket.type = (telemetry_packet_type_t) byte;
            parseState = PARSE_STATE_LEN;
            break;
        case PARSE_STATE_LEN:
            rxPacket.len = byte;
            parseState = PARSE_STATE_DATA;
            if (rxPacket.len == 0)
            {
                foundPacket = true;
            }
            break;
        case PARSE_STATE_DATA:
            rxPacket.data[parsedDataBytes++] = byte;
            if (parsedDataBytes >= rxPacket.len)
            {
                foundPacket = true;
            }
            break;
        default:
            resetParseState();
            break;
    }
    if (foundPacket)
    {
        memcpy(packet, &rxPacket, sizeof(telemetry_rx_packet_t));
        resetParseState();
    }
    return foundPacket;
}

void TelemetryPacketParser::resetParseState()
{
    parseState = PARSE_STATE_TYPE;
    parsedDataBytes = 0;
}
