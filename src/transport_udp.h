#ifndef TransportUDP_H
#define TransportUDP_H

#include <stdint.h>

class TransportUDP
{
    public:
        TransportUDP(const int port);
        void init(const char* remoteIP);
        void deInit();
        bool isInitialized();

        virtual bool readByte(uint8_t *byte, uint32_t timeoutMs) = 0;
        virtual uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) = 0;
        virtual uint32_t writeBytes(const uint8_t *data, uint32_t len) = 0;

        protected:
            char remoteIP[17];
            const int port;
            bool initialized;

            virtual void doInit() = 0;
            virtual void doDeInit() = 0;
};

#endif // TransportUDP
