#ifndef CAMERA_ESP32_H
#define CAMERA_ESP32_H

#include "camera.h"

#include "esp_camera.h"
#include <WiFi.h>
#include "esp32_transport_udp.h"

#include "apriltag.h"

class CameraESP32 : public Camera {
public:
    using Camera::Camera;
    uint8_t getTagsDetected(tag_position* tags) override;

protected:
    bool doInit() override;
    bool doCapture() override;
    const char* name() override;

    void sendImageOverUDP();

private:
    camera_fb_t* buffers[2];  // ✅ Double buffer for frames
    volatile int activeBuffer = 0;  // ✅ Track which buffer is active
    TaskHandle_t udpTaskHandle;  // ✅ FreeRTOS task for UDP sending
    SemaphoreHandle_t bufferMutex;
    uint32_t thrown_frames;

    apriltag_family_t* tf;
    apriltag_detector_t* td;

    static void udpSenderTask(void* arg);  // ✅ FreeRTOS task function

    bool backBufferEmpty(const TickType_t timeoutTicks = portMAX_DELAY);
    void putToBackBuffer(camera_fb_t* fb);
    void swapBuffers();
    void sendFrontBuffer();
    camera_fb_t* getActiveBuffer();
};

    

#endif