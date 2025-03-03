#include "esp32_camera.h"

#include "img_converters.h"
#include "esp_camera.h"
#include <WiFi.h>
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

#include "log.h"


// udp.init("192.168.0.195");


bool CameraESP32::doInit()
{
    if (!configureCamera())
    {
        error("Failed to initialize camera!\n");
        return false;
    }

    return true;
};


bool CameraESP32::doCapture()
{
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb)
    {
        printf("NO FB\n");
        return false;
    }

    detectTagsInImage((uint32_t) fb->width, (uint32_t) fb->height, (uint32_t) fb->width, fb->buf);

    uint8_t *jpeg_buf = NULL;
    size_t jpeg_len = 0;

    bool success = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_GRAYSCALE, 10, &jpeg_buf, &jpeg_len);
    if (!success)
    {
        printf("JPEG compression failed!!\n");
        return false;
    }

    esp_camera_fb_return(fb);

    sendImageOverUDP(jpeg_buf, jpeg_len);

    free(jpeg_buf);

    return true;
};

void CameraESP32::sendImageOverUDP(const uint8_t* buf, const uint32_t len)
{
    // TODO: Don't need this in future...
    if (!udp->isInitialized())
    {
        return;
    }

    // Send image over UDP
    uint32_t t0 = micros();

    const uint16_t CHUNK_SIZE = 1024;  // UDP max safe payload size
    uint32_t total_size = len;
    uint32_t num_chunks = (total_size / CHUNK_SIZE) + (total_size % CHUNK_SIZE ? 1 : 0);

    // Send image in chunks
    for (uint32_t i = 0; i < num_chunks; i++)
    {
        uint32_t offset = i * CHUNK_SIZE;
        uint32_t chunk_size = (offset + CHUNK_SIZE > total_size) ? (total_size - offset) : CHUNK_SIZE;
        
        udp->writeBytes(buf + offset, chunk_size);
    }
    uint32_t tx_dt_us = micros() - t0;
}

const char* CameraESP32::name()
{
    return "ESP32-S3";
};

uint8_t CameraESP32::getTagsDetected(tag_position* tags)
{
    memcpy(tags, &m_tags_detected, (m_nbr_of_tags_detected*sizeof(tag_position)));
    return m_nbr_of_tags_detected;
}

bool CameraESP32::configureCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 16000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_GRAYSCALE; // Required for AprilTag processing
    config.grab_mode = CAMERA_GRAB_LATEST; // Has to be in this mode, or detection will be lag
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) 
    {
        return false;
    }

    return true;
}

void CameraESP32::initializeTagDetection()
{
    // Tag detection
    tf = tag16h5_create();
    //tf = tagStandard41h12_create();

    // Create AprilTag detector object
    td = apriltag_detector_create();
    
    // Add tag family to the detector
    apriltag_detector_add_family(td, tf);

    // Tag detector configs
    td->quad_sigma = 0.6;
    td->quad_decimate = 4.0;
    td->refine_edges = 1;
    td->decode_sharpening = 0.25;
    td->nthreads = 1;
    td->debug = 0;
}


void CameraESP32::detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf)
{
    image_u8_t im = {
        .width = (int32_t) width,
        .height = (int32_t) height,
        .stride = (int32_t) stride,
        .buf = (uint8_t*) buf
    };
  
    // Detect
    zarray_t *detections = apriltag_detector_detect(td, &im);

    m_nbr_of_tags_detected = zarray_size(detections);
    if (m_nbr_of_tags_detected > 0)
    {
        //printf("Detections: ");
        for (int i = 0; i < m_nbr_of_tags_detected; i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);
            
            tag_position* tag = &m_tags_detected[i];

            for (int point = 0; point < 4; point++)
            {
                tag->p[point].x = det->p[point][0];
                tag->p[point].y = det->p[point][1];
            }
            tag->center.x = det->c[0];
            tag->center.y = det->c[1];
            //printf("%d ", det->id);
        }
        //printf("\n");
    }
    else
    {
        //printf("No tag detected\n");
    }

    // Free memory
    apriltag_detections_destroy(detections);
}

void CameraESP32::setTagDetectionParams(const tag_detection_params_t* params)
{
    td->quad_decimate = params->quad_decimate;
    td->quad_sigma = params->quad_sigma;
    td->refine_edges = params->refine_edges;
    td->decode_sharpening = params->decode_sharpening;
}

tag_detection_params_t CameraESP32::getTagDetectionParams()
{
    return
    {
        .quad_decimate = td->quad_decimate,
        .quad_sigma = td->quad_sigma,
        .refine_edges = td->refine_edges,
        .decode_sharpening = td->decode_sharpening,
    };
}
