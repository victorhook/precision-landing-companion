#include "hal.h"
#include "WiFi.h"

static bool startWifiHotSpot = false;

void hal_init()
{
    if (startWifiHotSpot)
    {

    }
    else
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin("COMHEM_f0a079", "ftmkntn2");
        printf("Connecting to WiFi");
        while (WiFi.status() != WL_CONNECTED)
        {
            printf(".");
            delay(1000);
        }
        printf("Connected to WiFI, IP %s\n", WiFi.localIP().toString().c_str());
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