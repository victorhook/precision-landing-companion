#include "camera_linux.h"
#include <iostream>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "apriltag.h"
#include "tag16h5.h"

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

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, UDP_IP, &server_addr.sin_addr);

    // Open webcam
    cap = new cv::VideoCapture(0, cv::CAP_V4L2);

    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return;
    }
}

#define USE_WEBCAM

void CameraLinux::doCapture()
{
    // Capture from a real camera or load a test image
#ifdef USE_WEBCAM
    if (!cap->isOpened()) {
        std::cerr << "❌ Error: Could not open camera!" << std::endl;
        return;
    }
    
    *cap >> captured_image;  // Capture a frame
#else
    captured_image = cv::imread("apriltag.jpg", cv::IMREAD_GRAYSCALE);
#endif

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

    char tag_detection_buf[128];

    // Create tag family object
    apriltag_family_t *tf = tag16h5_create();

    // Create AprilTag detector object
    apriltag_detector_t *td = apriltag_detector_create();
    
    // Add tag family to the detector
    apriltag_detector_add_family(td, tf);

    // Tag detector configs
    // quad_sigma is Gaussian blur's sigma
    // quad_decimate: small number = faster but cannot detect small tags
    //                big number = slower but can detect small tags (or tag far away)
    // With quad_sigma = 1.0 and quad_decimate = 4.0, ESP32-CAM can detect 16h5 tag
    // from the distance of about 1 meter (tested with tag on screen. not on paper)
    td->quad_sigma = 0.0;
    td->quad_decimate = 4.0;
    td->refine_edges = 0;
    td->decode_sharpening = 0;
    td->nthreads = 1;
    td->debug = 0;

    image_u8_t* apriltag_image = image_u8_create_stride(captured_image.cols, captured_image.rows, captured_image.step);
    memcpy(apriltag_image->buf, captured_image.data, captured_image.total());

    zarray_t *detections = apriltag_detector_detect(td, apriltag_image);

    // Convert back to RGB
    if (captured_image.channels() == 1) {
        cv::cvtColor(captured_image, captured_image, cv::COLOR_GRAY2BGR);
    }

    // Print result
    bool tagDetected = zarray_size(detections) > 0;

    if (tagDetected)
    {
        sprintf(tag_detection_buf, "Detected %d aprilTags", zarray_size(detections));
        printf("Detected %d aprilTags: ", zarray_size(detections));
        for (int i = 0; i < zarray_size(detections); i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);

            // Get corner points
            cv::Point2f corners[4];
            for (int j = 0; j < 4; j++) {
                corners[j] = cv::Point2f(det->p[j][0], det->p[j][1]);
            }

            printf("%d, ", det->id);

            for (int j = 0; j < 4; j++) {
                cv::line(captured_image, corners[j], corners[(j+1) % 4], cv::Scalar(0, 255, 0), 10);
            }
        
            // Draw tag ID text
            cv::putText(captured_image, std::to_string(det->id), corners[0], 
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        }
        printf("\n");
    }
    else
    {
        sprintf(tag_detection_buf, "No tags detected");
    }

    // Cleanup
    apriltag_detections_destroy(detections);
    image_u8_destroy(apriltag_image);
    apriltag_detector_destroy(td);
    tag16h5_destroy(tf);
    
    char fps_buf[128];
    sprintf(fps_buf, "FPS: %d", getFps());

    cv::resize(captured_image, captured_image, cv::Size(640, 480), 0, 0, cv::INTER_NEAREST);

    cv::putText(captured_image, tag_detection_buf, cv::Point(30, 60), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
    cv::putText(captured_image, fps_buf, cv::Point(30, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
 

    /*

    // Load ArUco dictionary
    cv::Ptr<cv::aruco::Dictionary> aruco_dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();

    // Detect markers
    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;
    cv::aruco::detectMarkers(captured_image, aruco_dict, marker_corners, marker_ids, parameters);



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
    */
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
