#include "mavcom.h"
#include "log.h"
#include "globals.h"
#include "ap.h"

extern ap_t ap;

MavCom::MavCom()
: proxyPort(14550), proxyIsActive(false)
{
    m_ap = new TRANSPORT_AP_CLASS();
    m_udp = new TRANSPORT_UDP_CLASS(proxyPort);
    apTxQueue = new HAL_QUEUE_CLASS(mavlink_message_t, 10);
}

void MavCom::init()
{
    // Open communication to AP
    m_ap->init();

    setupMessageStreaming();
}


void MavCom::update_1hz()
{
    activateProxyIfNeeded();

    if ((hal_millis() - lastSysStatus) > 1000)
    {
        setupMessageStreaming();
    }

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
            //  New message found, handle message straight away
            handleMessage(msg_serial);

            // Send message over UDP
            uint16_t packet_len = mavlink_msg_to_send_buffer(mav_buf, &msg_serial);
            uint32_t tx_bytes = m_udp->writeBytes(mav_buf, packet_len);
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

    // Send pending TX packets
    while (!apTxQueue->isEmpty())
    {
        mavlink_message_t msg;
        if (!apTxQueue->pop(msg, 0))
        {
            break;
        }
        sendMavlinkMessage(&msg);
    }
}

bool MavCom::sendCommandInt(const uint16_t command, const uint8_t frame, const float param1, const float param2, const float param3, const float param4, const float param5, const float param6, const float param7)
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
    return apTxQueue->push(msg, 0);
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
    if (!proxyIsActive && telemetryClientIsConnected())
    {
        // Proxy is active yet, but we've got a connection on the telemetry,
        // let's start the mavproxy server

        if (m_udp != NULL)
        {
            // Close current activity
            m_udp->deInit();
        }
        proxyIsActive = true;
        getTelemetryClientIp(proxyIP);
        info("Starting mavproxy to %s:%d\n", proxyIP, proxyPort);
        m_udp->init(proxyIP);
    }
    
}

void MavCom::setupMessageStreaming()
{
    info("Requesting message stream rates from AP\n");
    // Let's configure AP to stream the messages that we want
    setMessageInterval(MAVLINK_MSG_ID_ATTITUDE, 10);
    setMessageInterval(MAVLINK_MSG_ID_RANGEFINDER, 10);
    setMessageInterval(MAVLINK_MSG_ID_SYS_STATUS, 10);
    setMessageInterval(MAVLINK_MSG_ID_BATTERY_STATUS, 10);
    setMessageInterval(MAVLINK_MSG_ID_RC_CHANNELS, 10);
    setMessageInterval(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 10);
}

void MavCom::setMessageInterval(const uint16_t msg_id, const int interval_hz)
{
    sendCommandIntNoQueue(MAV_CMD_SET_MESSAGE_INTERVAL, 0, msg_id, (interval_hz > 0) ? 1000000.0 / interval_hz : interval_hz);
}

void MavCom::sendCommandIntNoQueue(const uint16_t command, const uint8_t frame, const float param1, const float param2, const float param3, const float param4, const float param5, const float param6, const float param7)
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
    //printf("New mavlink message: %d\n", msg.msgid);
    
    switch(msg.msgid)
    {
        case MAVLINK_MSG_ID_HEARTBEAT:
            handleMessageHeartbeat(msg);
            break;
        case MAVLINK_MSG_ID_COMMAND_ACK:
            handleMessageCommandAck(msg);
            break;
        case MAVLINK_MSG_ID_VIBRATION:
            handleMessageVibration(msg);
            break;
        case MAVLINK_MSG_ID_SYS_STATUS:
            handleMessageSysStatus(msg);
            break;
        case MAVLINK_MSG_ID_ATTITUDE:
            handleMessageAttitude(msg);
            break;
        case MAVLINK_MSG_ID_VFR_HUD:
            handleMessageVfrHud(msg);
            break;
        case MAVLINK_MSG_ID_RANGEFINDER:
            handleMessageRangefinder(msg);
            break;
        case MAVLINK_MSG_ID_BATTERY_STATUS:
            handleMessageBatteryStatus(msg);
            break;
        case MAVLINK_MSG_ID_OPTICAL_FLOW:
            handleMessageOpticalFlow(msg);
            break;
        case MAVLINK_MSG_ID_RC_CHANNELS:
            handleMessageRcChannels(msg);
            break;
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            handleMessageGlobalPositionInt(msg);
            break;
        case MAVLINK_MSG_ID_LOCAL_POSITION_NED:
            handleMessageLocalPositionNed(msg);
            break;
        case MAVLINK_MSG_ID_ESTIMATOR_STATUS:
            handleMessageEstimatorStatus(msg);
            break;
        case MAVLINK_MSG_ID_LANDING_TARGET:
            handleMessageLandingTarget(msg);
        case MAVLINK_MSG_ID_CURRENT_MODE:
            handleMessageCurrentMode(msg);
            break;
        case MAVLINK_MSG_ID_PARAM_VALUE:
            handleMessageParamValue(msg);
            break;
        case MAVLINK_MSG_ID_STATUSTEXT:
            handleMessageStatusText(msg);
            break;
        // Ignore these
        case MAVLINK_MSG_ID_TIMESYNC:
        case MAVLINK_MSG_ID_SYSTEM_TIME:
                break;
        default:
            //printf("No handler for MAVLink message %d\n", msg.msgid);
            break;
    }
}

void MavCom::handleMessageHeartbeat(const mavlink_message_t& msg)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&msg, &heartbeat);
    ap.last_heartbeat = hal_millis();
    ap.mav_mode_flag = heartbeat.base_mode;
    ap.mav_state = heartbeat.system_status;
    ap.flight_mode = heartbeat.custom_mode;
    //printf("SYS_STATUS: %u\n", m_lastHeartbeat);
}

void MavCom::handleMessageCommandAck(const mavlink_message_t& msg)
{
    mavlink_command_ack_t command_ack;
    mavlink_msg_command_ack_decode(&msg, &command_ack);
    info_ap("ACK: %d\n", command_ack.command);
}

void MavCom::handleMessageParamValue(const mavlink_message_t& msg)
{
    mavlink_param_value_t param_value;
    mavlink_msg_param_value_decode(&msg, &param_value);
    char param_name[17];
    memcpy(param_name, param_value.param_id, 16);
    param_name[strnlen(param_name, 16)] = 0;
    info_ap("Param: %s: %f\n", param_name, param_value.param_value);
}

void MavCom::handleMessageStatusText(const mavlink_message_t& msg)
{
    char text[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN];
    mavlink_msg_statustext_get_text(&msg, text);
    info_ap("%s\n", text);
}

void MavCom::handleMessageVibration(const mavlink_message_t& msg)
{
    mavlink_vibration_t vibration;
    mavlink_msg_vibration_decode(&msg, &vibration);
}

void MavCom::handleMessageSysStatus(const mavlink_message_t& msg)
{
    mavlink_sys_status_t sys_status;
    mavlink_msg_sys_status_decode(&msg, &sys_status);
    ap.bat_perc = sys_status.battery_remaining;
    ap.bat_voltage = (float) sys_status.voltage_battery / 1000.0;
    lastSysStatus = hal_millis();
}

void MavCom::handleMessageAttitude(const mavlink_message_t& msg)
{
    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&msg, &attitude);
    ap.roll = attitude.roll;
    ap.pitch = attitude.pitch;
    ap.yaw = attitude.yaw;
    ap.roll_speed = attitude.rollspeed;
    ap.pitch_speed = attitude.pitchspeed;
    ap.yaw_speed = attitude.yawspeed;
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
    ap.rngfnd_dist_m = rangefinder.distance;
}

void MavCom::handleMessageBatteryStatus(const mavlink_message_t& msg)
{
    mavlink_battery_status_t battery_status;
    mavlink_msg_battery_status_decode(&msg, &battery_status);
}

void MavCom::handleMessageRcChannels(const mavlink_message_t& msg)
{
    mavlink_rc_channels_t rc_channels;
    mavlink_msg_rc_channels_decode(&msg, &rc_channels);
    ap.chan1_raw = rc_channels.chan1_raw;
    ap.chan2_raw = rc_channels.chan2_raw;
    ap.chan3_raw = rc_channels.chan3_raw;
    ap.chan4_raw = rc_channels.chan4_raw;
    ap.chan5_raw = rc_channels.chan5_raw;
    ap.chan6_raw = rc_channels.chan6_raw;
    ap.chan7_raw = rc_channels.chan7_raw;
    ap.chan8_raw = rc_channels.chan8_raw;
    ap.chan9_raw = rc_channels.chan9_raw;
    ap.chan10_raw = rc_channels.chan10_raw;
    ap.rssi = rc_channels.rssi;
}

void MavCom::handleMessageGlobalPositionInt(const mavlink_message_t& msg)
{
    mavlink_global_position_int_t global_position_int;
    mavlink_msg_global_position_int_decode(&msg, &global_position_int);
    ap.relative_alt = (float) global_position_int.relative_alt / 1000.0;
    ap.alt = (float) global_position_int.alt / 1000.0;
}

void MavCom::handleMessageLocalPositionNed(const mavlink_message_t& msg)
{
    mavlink_local_position_ned_t local_position_ned;
    mavlink_msg_local_position_ned_decode(&msg, &local_position_ned);
    ap.local_pos_x = local_position_ned.x;
    ap.local_pos_y = local_position_ned.y;
    ap.local_pos_z = local_position_ned.z;
    ap.local_pos_vx = local_position_ned.vx;
    ap.local_pos_vy = local_position_ned.vy;
    ap.local_pos_vz = local_position_ned.vz;
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
