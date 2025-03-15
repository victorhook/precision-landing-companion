#ifndef MAVCOM_H
#define MAVCOM_H

#include "mavlink.h"
#include "hal.h"

// Transport
#include "transport_udp.h"
#include "transport_ap.h"

#define AP_SYSTEM_ID    1
#define AP_COMPONENT_ID 1
#define MAVLINK_SYSTEM_ID    42
#define MAVLINK_COMPONENT_ID MAV_COMP_ID_ONBOARD_COMPUTER2


class MavCom
{
    public:
        MavCom();
        void init();
        void update_100hz();
        void update_10hz();
        void update_1hz();
        bool sendCommandInt(const uint16_t command, const uint8_t frame = 0, const float param1 = 0, const float param2 = 0, const float param3 = 0, const float param4 = 0, const float param5 = 0, const float param6 = 0, const float param7 = 0);

    private:
        uint32_t m_lastHeartbeat;
        uint32_t lastSysStatus;
        TransportUDP* m_udp;
        TransportAP* m_ap;
        bool proxyIsActive;
        char proxyIP[17];
        const uint16_t proxyPort;
        HAL_Queue<mavlink_message_t>* apTxQueue;

        void setupMessageStreaming();
        void sendCommandIntNoQueue(const uint16_t command, const uint8_t frame = 0, const float param1 = 0, const float param2 = 0, const float param3 = 0, const float param4 = 0, const float param5 = 0, const float param6 = 0, const float param7 = 0);
        void setMessageInterval(const uint16_t msg_id, const int interval_hz);
        void sendMavlinkMessage(const mavlink_message_t *msg);

        void activateProxyIfNeeded();

        void sendLandingTargetPacket(const landing_target_t* target);

        // -- MAVLink message handlers -- //
        void handleMessage(const mavlink_message_t& msg);
        void handleMessageHeartbeat(const mavlink_message_t& msg);
        void handleMessageCommandAck(const mavlink_message_t& msg);
        void handleMessageStatusText(const mavlink_message_t& msg);
        void handleMessageVibration(const mavlink_message_t& msg);
        void handleMessageAttitude(const mavlink_message_t& msg);
        void handleMessageBatteryStatus(const mavlink_message_t& msg);
        void handleMessageSysStatus(const mavlink_message_t& msg);
        void handleMessageVfrHud(const mavlink_message_t& msg);
        void handleMessageRangefinder(const mavlink_message_t& msg);
        void handleMessageOpticalFlow(const mavlink_message_t& msg);
        void handleMessageRcChannels(const mavlink_message_t& msg);
        void handleMessageGlobalPositionInt(const mavlink_message_t& msg);
        void handleMessageLocalPositionNed(const mavlink_message_t& msg);
        void handleMessageEstimatorStatus(const mavlink_message_t& msg);
        void handleMessageLandingTarget(const mavlink_message_t& msg);
        void handleMessageCurrentMode(const mavlink_message_t& msg);
        void handleMessageParamValue(const mavlink_message_t& msg);
};

#endif