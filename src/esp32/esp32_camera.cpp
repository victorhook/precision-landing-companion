#include "esp32_camera.h"

#include "esp_camera.h"
#include <WiFi.h>
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

#include "esp32/esp32_transport_udp.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "tagStandard41h12.h"
#include "tag16h5.h"


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

    config.frame_size = FRAMESIZE_VGA;
    config.pixel_format = PIXFORMAT_GRAYSCALE; // Required for AprilTag processing
    config.grab_mode = CAMERA_GRAB_LATEST; // Has to be in this mode, or detection will be lag
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;

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


    // Tag detection
    tf = tag16h5_create();
    //tf = tagStandard41h12_create();

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

        camera_fb_t* fb = self->getActiveBuffer();

        if (!fb || fb->len == 0)
        {
            // Should never occur
            return;
        }

        image_u8_t im = {
            .width = fb->width,
            .height = fb->height,
            .stride = fb->width,
            .buf = fb->buf
          };
      
        // Detect
        zarray_t *detections = apriltag_detector_detect(self->td, &im);
        bool tagDetected = zarray_size(detections) > 0;

        if (tagDetected)
        {
            printf("Detections: ");
            for (int i = 0; i < zarray_size(detections); i++) {
                apriltag_detection_t *det;
                zarray_get(detections, i, &det);
                printf("%d ", det->id);
            }
            printf("\n");
        }
        else
        {
            printf("No tag detected\n");
        }


        // Send the front buffer 
        //uint32_t t0 = micros();
        //self->sendFrontBuffer();
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
