#include "linux_camera.h"

#include <iostream>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "apriltag.h"
#include "tag16h5.h"
#include "tagStandard41h12.h"

#define UDP_IP "127.0.0.1"  // Change to your receiver’s IP
#define UDP_PORT 9095
#define CHUNK_SIZE 4096  // UDP max packet size


bool CameraLinux::doInit()
{
    std::cout << "Initializing Linux camera simulation, server at " << UDP_IP << ":" << UDP_PORT  << std::endl;

    // Set up UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating UDP socket" << std::endl;
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, UDP_IP, &server_addr.sin_addr);

    // Open webcam
    cap = new cv::VideoCapture(0, cv::CAP_V4L2);

    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return false;
    }

    // -- Apriltag detection -- //
    // Create tag family object
    //tf = tag16h5_create();
    tf = tagStandard41h12_create();

    // Create AprilTag detector object
    td = apriltag_detector_create();
    
    // Add tag family to the detector
    apriltag_detector_add_family(td, tf);

    // Tag detector configs
    // quad_sigma is Gaussian blur's sigma
    // quad_decimate: small number = faster but cannot detect small tags
    //                big number = slower but can detect small tags (or tag far away)
    // With quad_sigma = 1.0 and quad_decimate = 4.0, ESP32-CAM can detect 16h5 tag
    // from the distance of about 1 meter (tested with tag on screen. not on paper)
    td->quad_sigma = 1.0;
    td->quad_decimate = 4.0;
    td->refine_edges = 1;
    td->decode_sharpening = 0.25;
    td->nthreads = 1;
    td->debug = 0;

    return true;
}

#define USE_WEBCAM

bool CameraLinux::doCapture()
{
    // Capture from a real camera or load a test image
#ifdef USE_WEBCAM
    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return false;
    }
    
    *cap >> captured_image;  // Capture a frame
#else
    captured_image = cv::imread("apriltag.jpg", cv::IMREAD_GRAYSCALE);
#endif

    if (captured_image.empty())
    {
        std::cerr << "Error: Could not load ArUco marker image!" << std::endl;
        return false;
    }

    processImage();
    sendImageOverUDP();
    
    return true;
}

void CameraLinux::processImage()
{
    // Convert to grayscale
    if (captured_image.channels() == 3)
    {
        cv::cvtColor(captured_image, captured_image, cv::COLOR_BGR2GRAY);
    }

    char tag_detection_buf[128];

    image_u8_t* apriltag_image = image_u8_create_stride(captured_image.cols, captured_image.rows, captured_image.step);
    memcpy(apriltag_image->buf, captured_image.data, captured_image.total());

    //zarray_t *detections = apriltag_detector_detect(td, apriltag_image);
    targetDetector->detectTagsInImage(apriltag_image->width, apriltag_image->height, apriltag_image->stride, apriltag_image->buf);

    // Convert back to RGB
    if (captured_image.channels() == 1) {
        cv::cvtColor(captured_image, captured_image, cv::COLOR_GRAY2BGR);
    }

    cv::resize(captured_image, captured_image, cv::Size(640, 480), 0, 0, cv::INTER_NEAREST);
    image_u8_destroy(apriltag_image);
}

void CameraLinux::sendImageOverUDP()
{
    std::vector<uchar> encoded;
    cv::resize(captured_image, captured_image, cv::Size(320, 240)); // Resize for lower bandwidth

    if (!cv::imencode(".jpg", captured_image, encoded))
    {
        std::cerr << "JPEG encoding failed" << std::endl;
        return;
    }

    // Send data in chunks
    size_t total_size = encoded.size();
    //std::cout << "Sending " << total_size << " bytes over UDP" << std::endl;
    for (size_t i = 0; i < total_size; i += CHUNK_SIZE)
    {
        size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), total_size - i);
        sendto(sockfd, &encoded[i], chunk_size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
}

void CameraLinux::sendImageOverUDP(const uint8_t* buf, const uint32_t len)
{

}

const char* CameraLinux::name()
{
    return "Linux";
}
