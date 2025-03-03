#ifndef ESP32_Transport_AP_H
#define ESP32_Transport_AP_H

#include <transport_ap.h>
#include "esp32_hal.h"

/**
 * Transport class to handle low-level communication to Ardupilot.
 */
class TransportAP_ESP32 : public TransportAP
{
    public:
        TransportAP_ESP32();
        bool init() override;
        uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
        uint32_t writeBytes(const uint8_t *data, uint32_t len) override;

    private:
        HardwareSerial* serial;
};

#endif // ESP32_Transport_AP
