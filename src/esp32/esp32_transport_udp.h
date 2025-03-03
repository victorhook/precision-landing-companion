#ifndef TRANSPORT_UDP_ESP32_H
#define TRANSPORT_UDP_ESP32_H

#include "transport_udp.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

class TransportUDP_ESP32 : public TransportUDP {
public:
    using TransportUDP::TransportUDP;
    ~TransportUDP_ESP32();

    void doInit() override;
    void doDeInit() override;
    void closeSocket();

    bool readByte(uint8_t* byte, uint32_t timeoutMs);
    uint32_t readBytes(uint8_t* bytes, uint32_t maxLen, uint32_t timeoutMs);
    uint32_t writeBytes(const uint8_t* data, uint32_t len);

private:
    WiFiUDP udp;
    volatile bool running;
};

#endif // TRANSPORT_UDP_ESP32_H
