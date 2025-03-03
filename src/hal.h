#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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

/* Abstract class for thread-safe queue */
template <typename T>
class HAL_Queue
{
public:
    virtual ~HAL_Queue() = default;

    virtual bool push(const T &item, uint32_t timeoutMs = 0) = 0;  // ✅ Now supports timeout
    virtual bool pop(T &item, uint32_t timeoutMs = 0) = 0;         // ✅ Now supports timeout
    virtual bool isEmpty() = 0;
    virtual bool isFull() = 0;
    virtual uint32_t maxSize() = 0;  // ✅ Returns queue capacity
};

// -- Platform specific -- //
#ifdef LINUX
    #include "linux/linux_camera.h"
    #define PLATFORM_NAME "linux"

    void setup();

    void loop();
#else
    #include <Arduino.h>
    #include "esp32/esp32_hal.h"
    #define PLATFORM_NAME "esp32-s3"
    #define TCP_SERVER_CLASS TransportTCP_Server_ESP32
    #define HAL_QUEUE_CLASS(type, maxSize) ESP32Queue<type>(maxSize)
    #define TRANSPORT_AP_CLASS TransportAP_ESP32
    #define TRANSPORT_UDP_CLASS TransportUDP_ESP32
    #define CAMERA_CLASS CameraESP32
#endif

#endif