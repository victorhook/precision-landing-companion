#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>


/* Delay for a specified number of milliseconds */
void hal_delay(const uint32_t ms);

/* Delay for a specified number of microseconds */
void hal_delayMicroseconds(const uint32_t us);

/* Get the current time in milliseconds since program start */
uint32_t hal_millis();

/* Get the current time in microseconds since program start */
uint32_t hal_micros();

/* Initializes the HAL */
void hal_init();

// -- Platform specific -- //
#ifdef LINUX
    #include "linux/linux_camera.h"
    #define PLATFORM_NAME "linux"

    void setup();

    void loop();
#else
    #include <Arduino.h>
    #include "esp32/esp32_camera.h"
    #include "esp32/esp32_transport_tcp_server.h"
    #include "esp32/esp32_transport_udp.h"
    #define PLATFORM_NAME "esp32-s3"
    #define TCP_SERVER_CLASS TransportTCP_Server_ESP32
#endif

#endif