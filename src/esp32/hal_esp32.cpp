#include "hal.h"


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