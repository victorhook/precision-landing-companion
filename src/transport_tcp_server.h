#ifndef TransportTCP_Server_H
#define TransportTCP_Server_H

#include <stdint.h>


class TransportTCP_Server
{
    public:
        TransportTCP_Server(const int port);

        virtual void init() = 0;
        virtual bool readByte(uint8_t *byte, uint32_t timeoutMs) = 0;
        virtual uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) = 0;
        virtual uint32_t writeBytes(const uint8_t *data, uint32_t len) = 0;
        virtual void flushTX() = 0;
        virtual bool clientConnected() = 0;
        /** Fills `ip` with the IP address of the remotely connected client. Returns true if successful. */
        virtual bool getClientIp(char ip[17]) = 0;
    
        protected:
            const int port;
};

#endif // TransportTCP_Server
