#include "main.h"

#include "mavcom.h"
#include "camera.h"
#include "WiFi.h"


MavCom mavcom;
Camera* camera;


void setup()
{
    printf("\n");
    printf("** Precision-Landing-Companion starting up **\n");
    printf("Platform: %s\n", PLATFORM_NAME);
    printf("Initializing drivers\n");

    #ifdef LINUX
        camera = new CameraLinux();
    #else
        camera = new CameraESP32();
    #endif

    WiFi.mode(WIFI_STA);
    WiFi.begin("COMHEM_f0a079", "ftmkntn2");
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    Serial.println(WiFi.localIP());

    //mavcom.init();
    camera->init();

    printf("Starting main loop\n");
    printf("\n");
}

uint32_t frame = 0;
static uint32_t next_frame = 0;
static uint32_t frame_period_us = 1000;
static void print_statistics();

void loop()
{
    frame++;

    if (frame % 10 == 0)
    {
        //mavcom.update_100hz();
    }
    if (frame % 100 == 0)
    {
        //mavcom.update_10hz();
    }
    if (frame % 1000 == 0)
    {
        //mavcom.update_1hz();
    }
    if (frame % 33 == 0)
    {
        uint32_t t0 = micros();
        camera->capture();
        uint32_t dt = micros() - t0;
        //printf("DT: %u us\n", dt);
    }

    print_statistics();

    next_frame += frame_period_us;
    long time_until_next_frame = ((long) next_frame - (long) hal_micros());
    //printf("%d, %lu\n", frame, time_until_next_frame);
    if (time_until_next_frame > 0)
    {
        hal_delayMicroseconds(time_until_next_frame);
    }
}


static void print_statistics()
{
    static uint32_t last_print = 0;
    if ((millis() - last_print) > 1000)
    {
        printf("FPS: %d, Thrown: %d, IP: %s\n", camera->getFps(), camera->getThrownFrames(), WiFi.localIP().toString().c_str());
        last_print = millis();
    }
}