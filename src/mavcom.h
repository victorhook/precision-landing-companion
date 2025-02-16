#ifndef MAVCOM_H
#define MAVCOM_H

#include "mavlink.h"
#include "main.h"
#include "mavcom_udp.h"

#define MAVLINK_SYSTEM_ID    42
#define MAVLINK_COMPONENT_ID MAV_COMP_ID_ONBOARD_COMPUTER2

class MavCom
{
    public:
        MavCom();
        void init();
        virtual void doInit() = 0;
        void update_100hz();
        void update_10hz();
        void update_1hz();

    protected:
        virtual void sendData(const uint8_t *data, uint32_t len) = 0;
        virtual bool readByteBlocking(uint8_t &byte, uint32_t timeout_ms) = 0;
        virtual uint32_t readBytes(uint8_t *buffer, uint32_t len, uint32_t max_len) = 0;

    private:
        uint32_t m_lastHeartbeat;
        MavComUDP m_udp;

        void handleMessage(const mavlink_message_t& msg);

        void handleMessageHeartbeat(const mavlink_message_t& msg);
        void handleMessageStatusText(const mavlink_message_t& msg);
        void handleMessageVibration(const mavlink_message_t& msg);
        void handleMessageVfrHud(const mavlink_message_t& msg);
        void handleMessageRangefinder(const mavlink_message_t& msg);
        void handleMessageOpticalFlow(const mavlink_message_t& msg);
        void handleMessageEstimatorStatus(const mavlink_message_t& msg);
        void handleMessageLandingTarget(const mavlink_message_t& msg);
        
};

#endif