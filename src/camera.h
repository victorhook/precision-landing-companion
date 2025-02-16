#ifndef CAMERA_H
#define CAMERA_H

#include "main.h"


class Camera
{
    public:
        Camera();
        void init();
        void capture();
        int getFps();

    protected:
        virtual void doInit() = 0;
        virtual void doCapture() = 0;
        virtual const char* name() = 0;

    private:
        int m_fps_counter;
        int m_fps;
        uint32_t m_last_fps_update;
};

#endif