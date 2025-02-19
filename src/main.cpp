#include "main.h"

#include "mavcom.h"
#include "camera.h"


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

    mavcom.init();
    camera->init();

    printf("Starting main loop\n");
    printf("\n");
}

uint32_t frame = 0;
static uint32_t next_frame = 0;
static uint32_t frame_period_us = 1000;

void loop()
{
    frame++;

    if (frame % 10 == 0)
    {
        mavcom.update_100hz();
    }
    if (frame % 100 == 0)
    {
        mavcom.update_10hz();
    }
    if (frame % 1000 == 0)
    {
        mavcom.update_1hz();
    }
    if (frame % 33 == 0)
    {
        camera->capture();
    }


    next_frame += frame_period_us;
    long time_until_next_frame = ((long) next_frame - (long) hal_micros());
    //printf("%d, %lu\n", frame, time_until_next_frame);
    if (time_until_next_frame > 0)
    {
        hal_delayMicroseconds(time_until_next_frame);
    }
}
