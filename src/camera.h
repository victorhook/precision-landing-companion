#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include "transport_udp.h"
#include "target_detector.h"


typedef struct
{
    uint16_t img_width;
    uint16_t img_height;
    uint8_t fov;
}__attribute__((packed)) camera_meta_data_t;

class Camera
{
    public:
        Camera(TargetDetector* targetDetector);
        bool init();
        void capture();
        int getFps();
        uint32_t getThrownFrames();
        camera_meta_data_t getMetaData();

    protected:
        bool m_isInitialized;
        uint32_t m_thrown_frames;
        virtual bool doInit() = 0;
        virtual bool doCapture() = 0;
        virtual const char* name() = 0;
        virtual void sendImageOverUDP(const uint8_t* buf, const uint32_t len) = 0;
        
        TransportUDP* udp;
        TargetDetector* targetDetector;

    private:
        int m_fps_counter;
        int m_fps;
        uint32_t m_last_fps_update;

        void initUdpBroadcastIfNeeded();
};

#endif