#include "telemetry.h"
#include "hal.h"

#include "log.h"
#include "ap.h"


typedef enum
{
    ACTION_ARM_CHECK = 0X01,
    ACTION_ARM       = 0x02,
    ACTION_DISARM    = 0x03,
    ACTION_TAKEOFF   = 0x04,
    ACTION_LAND      = 0x05,
    ACTION_REBOOT    = 0X06,
    ACTION_REBOOT_AP = 0x07
} action_t;
    
extern ap_t ap;



Telemetry::Telemetry(const int tcpPort)
{
    tcpServer = new TCP_SERVER_CLASS(tcpPort);
    logQueue = new HAL_QUEUE_CLASS(telemetry_log_t, 10);
}

void Telemetry::init(Camera* camera, TargetDetector* targetDetector, MavCom* mavcom)
{
    this->camera = camera;
    this->targetDetector = targetDetector;
    this->mavcom = mavcom;
    tcpServer->init();
    status.upTimeMs = hal_millis();
    status.cameraFps = 0;
    lastTelemetryPing = hal_millis();
}

void Telemetry::sendTelemetryPacket(const telemetry_packet_type_t packetType, const uint8_t* data, const uint32_t len)
{
    // Construct packet
    static telemetry_packet_t tx;

    // Byte 0-1 is magic
    tx.magic = 0x1234;
    // Byte 2 is packet type
    tx.type = packetType;
    // Byte 3-4 is packet length
    tx.len = len;

    memcpy(&tx.payload, data, tx.len);

    tcpServer->writeBytes((uint8_t*) &tx, TELEMETRY_PACKET_HEADER_SIZE+tx.len);
    tcpServer->flushTX();
}

void Telemetry::update()
{
    if (!tcpServer->clientConnected())
    {
        return;
    }

    static uint8_t buf[512];
    telemetry_rx_packet_t packet;

    // Read potential incoming packets
    int bytesRead = tcpServer->readBytes(buf, 100, 0);
    for (int i = 0; i < bytesRead; i++)
    {
        if (telemParser.parseByte(buf[i], &packet))
        {
            //info("RX PKT: %d (len=%d)\n", packet.type, packet.len);
            handleTelemetryCommand(&packet);
        }
    }

    // Send outgoing data
    const int header_size = 3;
    uint32_t packetSize = header_size + sizeof(telemetry_status_t);
    
    status.upTimeMs = hal_millis();
    status.cameraFps = (uint8_t) camera->getFps();
    status.freeHeap = hal_get_free_heap();
    tag_detection_params_t detection_params = targetDetector->getTagDetectionParams();
    status.quad_decimate = detection_params.quad_decimate;
    status.quad_sigma = detection_params.quad_sigma;
    status.refine_edges = detection_params.refine_edges;
    status.decode_sharpening = detection_params.decode_sharpening;

    // Copy ap struct to telemetry. This requires all fields to be packed identical... :)
    memcpy(&status.ap_last_heartbeat, &ap, sizeof(ap_t));

    sendTelemetryPacket(TELEMETRY_PACKET_STATUS, (uint8_t*) &status, sizeof(telemetry_status_t));

    // Empty log queue
    while (!logQueue->isEmpty())
    {
        telemetry_log_t log;
        if (!logQueue->pop(log, 0))
        {
            break;
        }
        sendTelemetryPacket(TELEMETRY_PACKET_LOG, (uint8_t*) &log, sizeof(telemetry_log_t));
    }


    // Check tag detections
    static tag_t tags[10];
    uint8_t tags_detected = targetDetector->getTagsDetected(tags);
    uint32_t all_tags_size = tags_detected * sizeof(tag_t);
    
    // Tag detection packet:
    // First byte: nbr of tags
    // Second byte: Locked yes/no
    // Locked tag (0s if no lock)
    // Landing target (`landing_target_t`)
    // All tags
    static tag_t lockedTag;
    bool hasLock = targetDetector->getLockedTag(&lockedTag);
    landing_target_t target;
    if (hasLock)
    {
        targetDetector->getLandingTarget(&target);
        //info(" -> [%d] (%f, %f) (%f, %f)\n", target.id, target.angle_x, target.angle_y, target.distance, target.size_x, target.size_y);
    }
    else
    {
        memset(&target, 0, sizeof(landing_target_t));
    }
    
    // Header
    buf[0] = tags_detected;
    buf[1] = (hasLock) ? 1 : 0;
    // Locked tag
    memcpy(&buf[2], &lockedTag, sizeof(tag_t));
    // Landing target
    memcpy(&buf[2 + sizeof(tag_t)], &target, sizeof(landing_target_t));
    // All of tags
    memcpy(&buf[2 + sizeof(tag_t) + sizeof(landing_target_t)], &tags, all_tags_size);
    
    uint32_t packet_size = 2 + sizeof(tag_t) + sizeof(landing_target_t) + all_tags_size;
    sendTelemetryPacket(TELEMETRY_PACKET_TAGS, buf, packet_size);

    return;
    // Disconnect client if we haven't received a ping within reasonable time
    if ((hal_millis() - lastTelemetryPing) > 3000)
    {
        // Client might be stuck connected
        info("Client inactive, disconnecting\n");
        tcpServer->disconnectClient();
    }
}

bool Telemetry::sendLogMsg(const log_level_t level, const log_group_t group, const char* msg)
{
    // Create new log block and put to queue
    telemetry_log_t log;
    log.level = level;
    log.group = group;
    log.timestamp = hal_millis();
    strncpy(log.msg, msg, TELEMTRY_LOG_MSG_MAX_SIZE);
    log.msg[TELEMTRY_LOG_MSG_MAX_SIZE] = 0; // Null termination

    return logQueue->push(log, 0);
}

void Telemetry::handleTelemetryCommand(const telemetry_rx_packet_t* pkt)
{
    tag_detection_params_t* detection_params;
    action_t action;

    switch (pkt->type)
    {
        case TELEMETRY_CMD_SET_DETECTION_PARAMS:
            detection_params = (tag_detection_params_t*) pkt->data;
            targetDetector->setTagDetectionParams(detection_params);
            break;
        case TELEMETRY_CMD_ACTION:
            action = (action_t) pkt->data[0];
            switch (action)
            {
                case ACTION_ARM_CHECK:
                    info("Running pre-arm check\n");
                    mavcom->sendCommandInt(MAV_CMD_RUN_PREARM_CHECKS);
                    break;
                case ACTION_ARM:
                    info("Arming\n");
                    break;
                case ACTION_DISARM:
                    info("Disarming\n");
                    break;
                case ACTION_TAKEOFF:
                    info("Takeoff\n");
                    break;
                case ACTION_LAND:
                    info("Landing\n");
                    break;
                case ACTION_REBOOT:
                    info("Rebooting\n");
                    // Add some delay to ensure logs gets through
                    hal_delay(500);
                    hal_reboot();
                    break;
                case ACTION_REBOOT_AP:
                    mavcom->sendCommandInt(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 0, 1);
                    info("Rebooting AP\n");
                    break;
                default:
                    warning("Unknown action %d\n", action);
                    break;
            }
            break;
        case TELEMETRY_PACKET_PING:
            lastTelemetryPing = hal_millis();
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
