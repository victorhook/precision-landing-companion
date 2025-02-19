#ifndef HAL_H
#define HAL_H

#include <stdint.h>

// Delay for a specified number of milliseconds
void hal_delay(const uint32_t ms);

void hal_delayMicroseconds(const uint32_t us);

// Get the current time in milliseconds since program start
uint32_t hal_millis();

// Get the current time in microseconds since program start
uint32_t hal_micros();


// -- Platform specific -- //
#ifdef LINUX
    #define PLATFORM_NAME "linux"

    void setup();

    void loop();
#else
    #include <Arduino.h>
    #define PLATFORM_NAME "esp32-s3"
#endif

#endif