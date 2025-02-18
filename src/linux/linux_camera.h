#ifndef CAMERA_LINUX_H
#define CAMERA_LINUX_H

#include "camera.h"
#include <opencv2/opencv.hpp>
#include <netinet/in.h>

#include "apriltag.h"


class CameraLinux : public Camera
{
public:
    bool doInit() override;
    void doCapture() override;
    void run(); // Continuously captures, processes, and streams images
    const char* name() override;

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
