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

    targetDetector->detectTagsInImage((uint32_t) fb->width, (uint32_t) fb->height, (uint32_t) fb->width, fb->buf);

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
    uint32_t t0 = hal_micros();

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
    uint32_t tx_dt_us = hal_micros() - t0;
}

const char* CameraESP32::name()
{
    return "ESP32-S3";
};

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
    config.frame_size = FRAMESIZE_240X240;
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
