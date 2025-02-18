#ifndef HAL_LINUX_H
#define HAL_LINUX_H

#include <stdint.h>

#define PLATFORM_NAME "linux"

// Delay for a specified number of milliseconds
void delay(const uint32_t ms);

void delayMicroseconds(const uint32_t us);

// Get the current time in milliseconds since program start
uint32_t millis();

// Get the current time in microseconds since program start
uint32_t micros();


void setup();

void loop();

#endif