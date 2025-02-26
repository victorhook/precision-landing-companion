#include "camera.h"

#include "hal.h"


Camera::Camera()
: m_isInitialized(0), m_last_fps_update(0), m_thrown_frames(0)
{

}

bool Camera::init()
{
   m_isInitialized = doInit();

   initializeTagDetection();

   return m_isInitialized;
}


void Camera::capture()
{
   if (!m_isInitialized)
   {
      return;
   }

   doCapture();
   m_fps_counter++;

   if ((hal_millis() - m_last_fps_update) > 1000)
   {
      m_last_fps_update = hal_millis();
      m_fps = m_fps_counter;
      m_fps_counter = 0;
   }
}


int Camera::getFps()
{
   return m_fps;
}


uint32_t Camera::getThrownFrames()
{
   return m_thrown_frames;
}