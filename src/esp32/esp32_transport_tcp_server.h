#ifndef TransportTCP_Server_ESP32_H
#define TransportTCP_Server_ESP32_H

#include "transport_tcp_server.h"
#include <WiFiClient.h>
#include <WiFi.h>

#include <freertos/FreeRTOS.h>

class TransportTCP_Server_ESP32 : public TransportTCP_Server
{
    public:
        TransportTCP_Server_ESP32(const int port);
        void init() override;
        bool readByte(uint8_t *byte, uint32_t timeoutMs) override;
        uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
        uint32_t writeBytes(const uint8_t *data, uint32_t len) override;
        void flushTX() override;

    private:
        WiFiClient client;
        WiFiServer server;
        bool isInitialized;
        TaskHandle_t receiverTaskHandle;

        void initServer();
        static void receiverThread(void* arg);
};

#endif // TransportTCP_Server_ESP32
