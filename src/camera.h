#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

typedef struct
{
    float x;
    float y;
} point_2f;


typedef struct
{
    // The center of the detection in image pixel coordinates.
    point_2f center;

    // The corners of the tag in image pixel coordinates. These always
    // wrap counter-clock wise around the tag.
    point_2f x1;
    point_2f y1;
    point_2f x2;
    point_2f y2;
} tag_position;


class Camera
{
    public:
        Camera();
        bool init();
        void capture();
        int getFps();
        uint32_t getThrownFrames();

    protected:
        bool m_isInitialized;
        uint32_t m_thrown_frames;
        virtual bool doInit() = 0;
        virtual bool doCapture() = 0;
        virtual const char* name() = 0;

    private:
        int m_fps_counter;
        int m_fps;
        uint32_t m_last_fps_update;
};

#endif