#include "esp32_transport_tcp_server.h"

#include <WiFi.h>
#include "log.h"


TransportTCP_Server_ESP32::TransportTCP_Server_ESP32(const int port)
: TransportTCP_Server(port), server(WiFiServer(port))
{

}

void TransportTCP_Server_ESP32::init()
{
    isInitialized = false;

    // ✅ Start FreeRTOS UDP sender task
    xTaskCreatePinnedToCore(
    receiverThread,  // Task function
    "TCP_Server",   // Task name
    4096,           // Stack size (4KB)
    this,           // Pass CameraESP32 instance
    1,              // Priority (1 = Low)
    &receiverTaskHandle, // Task handle
    1               // Pin to core 1
);
}

bool TransportTCP_Server_ESP32::readByte(uint8_t *byte, uint32_t timeoutMs)
{
    if (client && client.available())
    {
        int res = client.read();
        if (res == -1)
        {
            return false;
        }
        *byte = res;
        return true;
    }
    return false;
}

uint32_t TransportTCP_Server_ESP32::readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs)
{
    if (client && client.available())
    {
        return client.read(bytes, maxLen);
    }
    return 0;
}

uint32_t TransportTCP_Server_ESP32::writeBytes(const uint8_t *data, uint32_t len)
{
    if (client && client.connected())
    {
        return client.write(data, len);
    }
    return 0;
}

void TransportTCP_Server_ESP32::initServer()
{
    info("Initializing TCP server on %s:%d\n", WiFi.localIP().toString().c_str(), port);
    isInitialized = true;
    server.begin();
}

void TransportTCP_Server_ESP32::receiverThread(void* arg)
{
    TransportTCP_Server_ESP32* self = static_cast<TransportTCP_Server_ESP32*>(arg);

    while (1)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            delay(10);
            continue;
        }

        if (!self->isInitialized)
        {
            self->initServer();
        }

        if (!self->client || !self->client.connected())  // ✅ Check if client is connected
        {
            self->client = self->server.available();  // ✅ Accept a new client connection
            if (self->client) 
            {
                info("New client connected\n");
            }
        }

        if (self->client.available())  // ✅ Check for incoming data
        {
        }

        delay(5);
    }
}

void TransportTCP_Server_ESP32::flushTX()
{
    if (client && client.available())
    {
        client.flush();
    }
}

void TransportTCP_Server_ESP32::disconnectClient()
{
    if (client && client.connected())
    {
        info("Disconnecting TCP client\n");
        client.stop();
    }
}

bool TransportTCP_Server_ESP32::clientConnected()
{
    return (client && client.connected());
}


bool TransportTCP_Server_ESP32::getClientIp(char ip[17])
{
    if (!clientConnected())
    {
        ip[0] = '\0';
        return false;
    }

    strncpy(ip, client.remoteIP().toString().c_str(), 17);
    return true;
}
