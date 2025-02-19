#include "esp32_camera.h"

#include "esp_camera.h"
#include <WiFi.h>
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

#include "esp32/esp32_transport_udp.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>


TransportUDP_ESP32 udp("192.168.0.195", 9095);

bool CameraESP32::doInit()
{
    // Init UDP broadcast
    udp.init();

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
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;  // for streaming
    //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    //                      for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        if (psramFound())
        {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else
        {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }
    else {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
        #if CONFIG_IDF_TARGET_ESP32S3
            config.fb_count = 2;
        #endif
    }

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) 
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) 
    {
        s->set_vflip(s, 1);        // flip it back
        s->set_brightness(s, 1);   // up the brightness just a bit
        s->set_saturation(s, -2);  // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

     // ✅ Start FreeRTOS UDP sender task
     xTaskCreatePinnedToCore(
        udpSenderTask,  // Task function
        "UDP_Sender",   // Task name
        4096,           // Stack size (4KB)
        this,           // Pass CameraESP32 instance
        1,              // Priority (1 = Low)
        &udpTaskHandle, // Task handle
        1               // Pin to core 1
    );
    bufferMutex = xSemaphoreCreateMutex();
    buffers[0] = NULL;
    buffers[1] = NULL;

    return true;
};

bool CameraESP32::doCapture()
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    fb = esp_camera_fb_get();
    if (!fb)
    {
        return false;
    }

    // Put image to back buffer if it's empty
    if (backBufferEmpty(pdMS_TO_TICKS(1)))
    {
        putToBackBuffer(fb);
    }
    else
    {
        // If we don't send the frame over UDP we must return it to the camera driver once done.
        // This is handled by the UDP transmitter on normal occasions.
        m_thrown_frames++;
        esp_camera_fb_return(fb);
    }

    return true;
};

const char* CameraESP32::name()
{
    return "ESP32-S3";
};

void CameraESP32::udpSenderTask(void* arg)
{
    CameraESP32* self = static_cast<CameraESP32*>(arg);

    while (true)
    {
        // Wait for back buffer to become empty
        while (self->backBufferEmpty()) {}

        // Once the back buffer has been filled, we'll swap it to frot
        self->swapBuffers();

        // Send the front buffer 
        //uint32_t t0 = micros();
        self->sendFrontBuffer();
        //uint32_t tx_dt_us = micros() - t0;
    }
}

void CameraESP32::sendFrontBuffer()
{
    camera_fb_t* fb = getActiveBuffer();

    if (!fb || fb->len == 0)
    {
        // Should never occur
        return;
    }

    const uint16_t CHUNK_SIZE = 1024;  // UDP max safe payload size
    uint32_t total_size = fb->len;
    uint32_t num_chunks = (total_size / CHUNK_SIZE) + (total_size % CHUNK_SIZE ? 1 : 0);

    // Send image in chunks
    for (uint32_t i = 0; i < num_chunks; i++)
    {
        uint32_t offset = i * CHUNK_SIZE;
        uint32_t chunk_size = (offset + CHUNK_SIZE > total_size) ? (total_size - offset) : CHUNK_SIZE;
        
        udp.writeBytes(fb->buf + offset, chunk_size);
    }
}

bool CameraESP32::backBufferEmpty(const TickType_t timeoutTicks)
{
    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    camera_fb_t* backBuffer = buffers[1 - activeBuffer];
    bool backBufferIsEmpty = backBuffer == NULL;
    xSemaphoreGive(bufferMutex);
    return backBufferIsEmpty;
}

void CameraESP32::putToBackBuffer(camera_fb_t* fb)
{
    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    buffers[1 - activeBuffer] = fb;
    xSemaphoreGive(bufferMutex);
}

void CameraESP32::swapBuffers()
{
    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    // Free previous buffer before we swap
    camera_fb_t* prevBuffer = buffers[activeBuffer];
    if (prevBuffer != NULL)
    {
        esp_camera_fb_return(prevBuffer);
    }
    buffers[activeBuffer] = NULL;

    // Once returned we'll swap buffers
    activeBuffer = 1 - activeBuffer;
    xSemaphoreGive(bufferMutex);
}

camera_fb_t* CameraESP32::getActiveBuffer()
{
    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    camera_fb_t* fb = buffers[activeBuffer];
    xSemaphoreGive(bufferMutex);
    return fb;
}
