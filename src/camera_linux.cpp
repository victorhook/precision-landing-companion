#include "camera_linux.h"
#include <iostream>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define UDP_IP "127.0.0.1"  // Change to your receiver’s IP
#define UDP_PORT 9095
#define CHUNK_SIZE 4096  // UDP max packet size

void CameraLinux::doInit()
{
    std::cout << "Initializing Linux camera simulation, server at " << UDP_IP << ":" << UDP_PORT  << std::endl;

    // Set up UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating UDP socket" << std::endl;
        return;
    }

    cap = new cv::VideoCapture(0, cv::CAP_V4L2);

    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, UDP_IP, &server_addr.sin_addr);
}

void CameraLinux::doCapture()
{
    // Capture from a real camera or load a test image
    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return;
    }
    
    *cap >> captured_image;  // Capture a frame

    //captured_image = cv::imread("aruco_marker.jpg", cv::IMREAD_GRAYSCALE);

    if (captured_image.empty())
    {
        std::cerr << "Error: Could not load ArUco marker image!" << std::endl;
        return;
    }

    processImage();
    sendImageOverUDP();
}

void CameraLinux::run()
{
    /*
    cap.open(0);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Could not open camera!" << std::endl;
        return;
    }

    while (true)
    {
        cap >> captured_image;
        if (captured_image.empty())
        {
            std::cerr << "Error: Could not capture image!" << std::endl;
            continue;
        }

        processImage();
        sendImageOverUDP();
        usleep(33000); // Approx 30 FPS
    }
    */
}

void CameraLinux::processImage()
{
    std::cout << "Processing image..." << std::endl;

    // Convert to grayscale
    if (captured_image.channels() == 3)
    {
        cv::cvtColor(captured_image, captured_image, cv::COLOR_BGR2GRAY);
    }

    // Load ArUco dictionary
    cv::Ptr<cv::aruco::Dictionary> aruco_dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();

    // Detect markers
    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;
    cv::aruco::detectMarkers(captured_image, aruco_dict, marker_corners, marker_ids, parameters);

    char fps_buf[128];
    sprintf(fps_buf, "FPS: %d", getFps());
    cv::putText(captured_image, fps_buf, cv::Point(30, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);

    if (!marker_ids.empty())
    {
        cv::aruco::drawDetectedMarkers(captured_image, marker_corners, marker_ids);

        // ✅ Add "ArUco Detected" text overlay
        cv::putText(captured_image, "ArUco Detected", cv::Point(30, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        
        std::cout << "Detected " << marker_ids.size() << " ArUco marker(s)!" << std::endl;
    }
    else
    {
        // ✅ Add "No ArUco" text overlay
        cv::putText(captured_image, "No ArUco", cv::Point(30, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);

        std::cout << "No ArUco markers detected." << std::endl;
    }
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
    std::cout << "Sending image over UDP" << std::endl;
    for (size_t i = 0; i < total_size; i += CHUNK_SIZE)
    {
        size_t chunk_size = std::min(static_cast<size_t>(CHUNK_SIZE), total_size - i);
        sendto(sockfd, &encoded[i], chunk_size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
}

const char* CameraLinux::name()
{
    return "Linux";
}
