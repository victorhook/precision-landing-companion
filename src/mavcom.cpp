#include "mavcom.h"
#include "log.h"


MavCom::MavCom()
: proxyPort(14550), proxyIsActive(false)
{
    m_ap = new TRANSPORT_AP_CLASS();
    m_udp = new TRANSPORT_UDP_CLASS(proxyPort);
}

void MavCom::init(Telemetry* telemetry)
{
    this->telemetry = telemetry;

    // Open communication to AP
    m_ap->init();
}


void MavCom::update_1hz()
{
    activateProxyIfNeeded();

    mavlink_message_t msg;

    mavlink_msg_heartbeat_pack(
        MAVLINK_SYSTEM_ID,
        MAVLINK_COMPONENT_ID,
        &msg,
        MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_AUTOPILOT_INVALID,
        0, 0,
        MAV_STATE_ACTIVE
    );
    sendMavlinkMessage(&msg);
}

void MavCom::update_10hz()
{
    activateProxyIfNeeded();

    landing_target_t landing_target;
    //sendLandingTargetPacket(&landing_target);
}

void MavCom::update_100hz()
{
    activateProxyIfNeeded();

    // Handles mavlink communication
    // 1. Read from Ardupilot
    //   -> Handle message if handler exists for given message
    //   -> Forward message to GCS (UDP broadcast)
    // 2. Read from GCS (UDP) and send messages to AP


    static uint8_t transport_buf[2048];
    static uint8_t mav_buf[MAVLINK_MAX_PACKET_LEN];
    uint32_t rx_bytes;

    // Read from serial
    rx_bytes = m_ap->readBytes(transport_buf, 2048, 0);
    mavlink_status_t status_serial;
    mavlink_message_t msg_serial;

    for (int i = 0; i < rx_bytes; i++)
    {
        if (mavlink_parse_char(0, transport_buf[i], &msg_serial, &status_serial))
        {
            handleMessage(msg_serial);
            uint16_t packet_len = mavlink_msg_to_send_buffer(mav_buf, &msg_serial);
            m_udp->writeBytes(mav_buf, packet_len);
            //printf("[SER] %u\n", msg_to_ap.seq);
        }
    }

    // Read from UDP
    rx_bytes = m_udp->readBytes(transport_buf, 2048, 0);
    mavlink_status_t status_udp;
    mavlink_message_t msg_udp;
    for (int i = 0; i < rx_bytes; i++)
    {
        if (mavlink_parse_char(1, transport_buf[i], &msg_udp, &status_udp))
        {
            uint16_t packet_len = mavlink_msg_to_send_buffer(mav_buf, &msg_udp);
            m_ap->writeBytes(mav_buf, packet_len);
            //printf("[UDP] %u\n", msg_to_gcs.seq);
        }
    }
}

// -- Private -- //
void MavCom::sendLandingTargetPacket(const landing_target_t* target)
{
    mavlink_message_t msg;
    mavlink_msg_landing_target_pack(
        MAVLINK_SYSTEM_ID,
        MAVLINK_COMPONENT_ID,
        &msg,
        hal_micros(),
        target->id,
        0,
        target->angle_x,
        target->angle_y,
        target->distance,
        target->size_x,
        target->size_y,
        0, 0, 0, 0, 0, 0
    );
    sendMavlinkMessage(&msg);
}

void MavCom::activateProxyIfNeeded()
{
    if (!proxyIsActive && telemetry->clientConnected())
    {
        // Proxy is active yet, but we've got a connection on the telemetry,
        // let's start the mavproxy server

        if (m_udp != NULL)
        {
            // Close current activity
            m_udp->deInit();
        }

        info("Starting mavproxy to %s:%d\n", proxyIP, proxyPort);
        proxyIsActive = true;
        telemetry->getClientIP(proxyIP);
        m_udp->init(proxyIP);
    }
    
}

void MavCom::setMessageInterval()
{

}

void MavCom::sendCommandInt(const uint16_t command, const uint8_t frame, const float param1, const float param2, const float param3, const float param4, const float param5, const float param6, const float param7)
{
    mavlink_message_t msg;
    mavlink_msg_command_int_pack(
        MAVLINK_SYSTEM_ID,
        MAVLINK_COMPONENT_ID,
        &msg,
        1, 1, frame,
        command,
        0, 0,
        param1, param2, param3, param4, param5, param6, param7
    );
    sendMavlinkMessage(&msg);
}


void MavCom::sendMavlinkMessage(const mavlink_message_t* msg)
{
    static uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);
    m_ap->writeBytes(buf, len);
}

void MavCom::handleMessage(const mavlink_message_t& msg)
{
    printf("New mavlink message: %d\n", msg.msgid);
    
    switch(msg.msgid)
    {
        case MAVLINK_MSG_ID_HEARTBEAT:
            handleMessageHeartbeat(msg);
            break;
        case MAVLINK_MSG_ID_VIBRATION:
            handleMessageVibration(msg);
            break;
        case MAVLINK_MSG_ID_VFR_HUD:
            handleMessageVfrHud(msg);
            break;
        case MAVLINK_MSG_ID_RANGEFINDER:
            handleMessageRangefinder(msg);
            break;
        case MAVLINK_MSG_ID_OPTICAL_FLOW:
            handleMessageOpticalFlow(msg);
            break;
        case MAVLINK_MSG_ID_ESTIMATOR_STATUS:
            handleMessageEstimatorStatus(msg);
            break;
        case MAVLINK_MSG_ID_LANDING_TARGET:
            handleMessageLandingTarget(msg);
        case MAVLINK_MSG_ID_CURRENT_MODE:
            handleMessageCurrentMode(msg);
            break;
        case MAVLINK_MSG_ID_STATUSTEXT:
            handleMessageStatusText(msg);
            break;
        // Ignore these
        case MAVLINK_MSG_ID_TIMESYNC:
        case MAVLINK_MSG_ID_SYSTEM_TIME:
                break;
        default:
            printf("No handler for MAVLink message %d\n", msg.msgid);
    }
}

void MavCom::handleMessageHeartbeat(const mavlink_message_t& msg)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&msg, &heartbeat);
    m_lastHeartbeat = hal_millis();
    //printf("SYS_STATUS: %u\n", m_lastHeartbeat);
}

void MavCom::handleMessageStatusText(const mavlink_message_t& msg)
{
    char text[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN];
    mavlink_msg_statustext_get_text(&msg, text);
    printf("AP: %s\n", text);
}

void MavCom::handleMessageVibration(const mavlink_message_t& msg)
{
    mavlink_vibration_t vibration;
    mavlink_msg_vibration_decode(&msg, &vibration);
}

void MavCom::handleMessageVfrHud(const mavlink_message_t& msg)
{
    mavlink_vfr_hud_t vfr_hud;
    mavlink_msg_vfr_hud_decode(&msg, &vfr_hud);
}

void MavCom::handleMessageRangefinder(const mavlink_message_t& msg)
{
    mavlink_rangefinder_t rangefinder;
    mavlink_msg_rangefinder_decode(&msg, &rangefinder);
}

void MavCom::handleMessageOpticalFlow(const mavlink_message_t& msg)
{
    mavlink_optical_flow_t optical_flow;
    mavlink_msg_optical_flow_decode(&msg, &optical_flow);
}

void MavCom::handleMessageEstimatorStatus(const mavlink_message_t& msg)
{
    mavlink_estimator_status_t estimator_status;
    mavlink_msg_estimator_status_decode(&msg, &estimator_status);
}

void MavCom::handleMessageLandingTarget(const mavlink_message_t& msg)
{
    mavlink_landing_target_t landing_target;
    mavlink_msg_landing_target_decode(&msg, &landing_target);
}

void MavCom::handleMessageCurrentMode(const mavlink_message_t& msg)
{
    mavlink_current_mode_t current_mode;
    mavlink_msg_current_mode_decode(&msg, &current_mode);
    printf("MODE: %d\n", current_mode.custom_mode);
}
