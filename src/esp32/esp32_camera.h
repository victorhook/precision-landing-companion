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
    void initializeTagDetection() override;
    void sendImageOverUDP(const uint8_t* buf, const uint32_t len) override;
    void detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf) override;

private:
    apriltag_family_t* tf;
    apriltag_detector_t* td;

    bool backBufferEmpty(const TickType_t timeoutTicks = portMAX_DELAY);
    void putToBackBuffer(camera_fb_t* fb);
    void swapBuffers();
    void sendFrontBuffer();
    camera_fb_t* getActiveBuffer();

    bool configureCamera();
};

    

#endif