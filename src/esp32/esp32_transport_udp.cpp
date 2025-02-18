#include "esp32_transport_udp.h"


TransportUDP_ESP32::~TransportUDP_ESP32()
{
}

void TransportUDP_ESP32::init()
{
  
}


bool TransportUDP_ESP32::readByte(uint8_t *byte, uint32_t timeoutMs)
{
    return true;
}

uint32_t TransportUDP_ESP32::readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs)
{
    return 0;
}

uint32_t TransportUDP_ESP32::writeBytes(const uint8_t *data, uint32_t len)
{
    return 0;
}
