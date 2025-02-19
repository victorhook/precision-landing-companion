#ifndef CAMERA_ESP32_H
#define CAMERA_ESP32_H

#include "camera.h"


class CameraESP32 : public Camera
{
    public:
        using Camera::Camera;
        bool doInit() override;
        bool doCapture() override;
        const char* name() override;
};

#endif