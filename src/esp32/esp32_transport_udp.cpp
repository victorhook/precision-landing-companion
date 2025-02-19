#include "esp32_transport_udp.h"

#define RX_QUEUE_LENGTH 2048  // Adjust the size as needed



TransportUDP_ESP32::~TransportUDP_ESP32() {
    running = false;
}

void TransportUDP_ESP32::init() {
    running = true;
}

bool TransportUDP_ESP32::readByte(uint8_t* byte, uint32_t timeoutMs) {
    return 0;
}

uint32_t TransportUDP_ESP32::readBytes(uint8_t* bytes, uint32_t maxLen, uint32_t timeoutMs) {
    return 0;
}

uint32_t TransportUDP_ESP32::writeBytes(const uint8_t* data, uint32_t len) {
    // Start a packet to the remote IP/port.
    if (udp.beginPacket(ip, port) != 1) {
        return 0;
    }
    int written = udp.write(data, len);
    if (written != (int)len) {
        // Optionally, handle partial writes.
    }
    if (udp.endPacket() != 1) {
        return 0;
    }
    return written;
}
