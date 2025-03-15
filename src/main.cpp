#include "hal.h"
#include "mavcom.h"
#include "camera.h"
#include "telemetry.h"
#include "target_detector.h"
#include "log.h"
#include "build.h"

MavCom mavcom;
Camera* camera;
Telemetry telemetry(9096);
TargetDetector targetDetector;

void setup()
{
    info("\n");
    info("** Precision-Landing-Companion starting up **\n");
    info("Platform: %s\n", PLATFORM_NAME);
    info("Build time: %s\n", BUILD_DATETIME);
    info("Initializing drivers\n");

    hal_init();

    // Instantiate and initialize camera driver
    camera = new CAMERA_CLASS(&targetDetector);
    camera->init();

    camera_meta_data_t meta_data = camera->getMetaData();
    targetDetector.init(meta_data.img_width, meta_data.img_height, meta_data.fov);

    telemetry.init(camera, &targetDetector, &mavcom);

    mavcom.init();

    info("Starting main loop\n");
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
        mavcom.update_100hz();
    }
    if (frame % 100 == 0)
    {
        //mavcom.update_10hz();
    }
    if (frame % 20 == 0)
    {
        //mavcom.update_10hz();
        telemetry.update();
    }
    if (frame % 1000 == 0)
    {
        mavcom.update_1hz();
    }
    if (frame % 33 == 0)
    {
        uint32_t t0 = hal_micros();
        camera->capture();
        uint32_t dt = hal_micros() - t0;
        //debug("CAM DT: %.1f ms\n", (float) dt / 1000.0);
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
    tag_t tags[10];
    static uint32_t last_print = 0;
    uint8_t tags_detected = targetDetector.getTagsDetected(tags);
    char ip[17];
    if ((hal_millis() - last_print) > 1000)
    {
        hal_get_ip(ip);
        printf("IP: %s, Heap: %d, ", ip, hal_get_free_heap());
        debug("FPS: %d, Tags: %d, Lock: %d\n", camera->getFps(), tags_detected, targetDetector.hasLock());
        last_print = hal_millis();
    }
}