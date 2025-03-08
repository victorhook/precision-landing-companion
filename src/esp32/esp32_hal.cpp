#include "esp32_hal.h"
#include "WiFi.h"
#include <cstring> // For memcpy
#include "credentials.h"

#include "log.h"

static bool startWifiHotSpot = false;


void hal_init()
{
    if (startWifiHotSpot)
    {
        WiFi.softAP(WIFI_HOTSPOT_SSID, WIFI_HOTSPOT_PASS);
        IPAddress myIP = WiFi.softAPIP();
        info("WiFi hotspot started: %s", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
        info("Connecting to WiFi");
        while (WiFi.status() != WL_CONNECTED)
        {
            info(".");
            delay(1000);
        }
        info("Connected to WiFI, IP %s\n", WiFi.localIP().toString().c_str());
    }
}

// Delay for a specified number of milliseconds
void hal_delay(const uint32_t ms)
{
    delay(ms);
}

// Delay for a specified number of microseconds
void hal_delayMicroseconds(const uint32_t us)
{
    delayMicroseconds(us);
}

// Get the current time in milliseconds since program start
uint32_t hal_millis()
{
    return millis();
}

// Get the current time in microseconds since program start
uint32_t hal_micros()
{
    return micros();
}

void hal_get_ip(char ip[17])
{
    String ipAddress;
    if (WiFi.getMode() == WIFI_MODE_STA)
    {
        ipAddress = WiFi.localIP().toString();
    }
    else
    {
        ipAddress = WiFi.softAPIP().toString();
    }
    strncpy(ip, ipAddress.c_str(), 16);
    ip[16] = 0;
}

uint32_t hal_get_free_heap()
{
    return ESP.getFreeHeap();
}
