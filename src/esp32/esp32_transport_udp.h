#ifndef TRANSPORT_UDP_ESP32
#define TRANSPORT_UDP_ESP32

#include <stdint.h>

#include "transport_udp.h"

class TransportUDP_ESP32 : public TransportUDP
{
public:
    ~TransportUDP_ESP32();
    void init() override;
    bool readByte(uint8_t *byte, uint32_t timeoutMs) override;
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
    uint32_t writeBytes(const uint8_t *data, uint32_t len) override;
};

#endif // TRANSPORT_UDP_ESP32
