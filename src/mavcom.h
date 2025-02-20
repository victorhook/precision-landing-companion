#ifndef MAVCOM_H
#define MAVCOM_H

#include "mavlink.h"
#include "hal.h"

// Transport
#include "transport_tcp.h"
#include "transport_udp.h"


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

    private:
        uint32_t m_lastHeartbeat;
        TransportUDP* m_udp;
        TransportTCP* m_tcp;


        void setMessageInterval();
        void sendCommandInt(const uint16_t command, const uint8_t frame = 0, const float param1 = 0, const float param2 = 0, const float param3 = 0, const float param4 = 0, const float param5 = 0, const float param6 = 0, const float param7 = 0);
        void sendMavlinkMessage(const mavlink_message_t *msg);

        // -- MAVLink message handlers -- //
        void handleMessage(const mavlink_message_t& msg);
        void handleMessageHeartbeat(const mavlink_message_t& msg);
        void handleMessageStatusText(const mavlink_message_t& msg);
        void handleMessageVibration(const mavlink_message_t& msg);
        void handleMessageVfrHud(const mavlink_message_t& msg);
        void handleMessageRangefinder(const mavlink_message_t& msg);
        void handleMessageOpticalFlow(const mavlink_message_t& msg);
        void handleMessageEstimatorStatus(const mavlink_message_t& msg);
        void handleMessageLandingTarget(const mavlink_message_t& msg);
        void handleMessageCurrentMode(const mavlink_message_t& msg);
};

#endif