#ifndef Transport_AP_H
#define Transport_AP_H

#include <stdint.h>

/**
 * Transport class to handle low-level communication to Ardupilot.
 */
class TransportAP
{
    public:
        virtual bool init() = 0;
        virtual uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) = 0;
        virtual uint32_t writeBytes(const uint8_t *data, uint32_t len) = 0;
};

#endif // Transport_AP
