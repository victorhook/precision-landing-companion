#ifndef TransportTCP_H
#define TransportTCP_H

#include <stdint.h>


class TransportTCP
{
    public:
        TransportTCP(const char* ip, const int port);

        virtual void init() = 0;
        virtual bool readByte(uint8_t *byte, uint32_t timeoutMs) = 0;
        virtual uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) = 0;
        virtual uint32_t writeBytes(const uint8_t *data, uint32_t len) = 0;
    
        protected:
            const char* ip;
            const int port;
};

#endif // TransportTCP
