#ifndef CAMERA_LINUX_H
#define CAMERA_LINUX_H

#include "camera.h"
#include <opencv2/opencv.hpp>
#include <netinet/in.h>

#include "apriltag.h"


class CameraLinux : public Camera
{
public:
    using Camera::Camera;

    bool doInit() override;
    bool doCapture() override;
    const char* name() override;
    void sendImageOverUDP(const uint8_t* buf, const uint32_t len) override;

private:
    cv::Mat captured_image;
    cv::VideoCapture* cap;

    apriltag_family_t* tf;
    apriltag_detector_t* td;

    int sockfd;
    struct sockaddr_in server_addr;

    void processImage();
    void sendImageOverUDP();
};

#endif // CAMERA_LINUX_H
