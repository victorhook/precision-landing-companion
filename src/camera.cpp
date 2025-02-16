#include "camera.h"

Camera::Camera()
: m_last_fps_update(0)
{

}

void Camera::init()
{
   doInit();
}


void Camera::capture()
{
   doCapture();
   m_fps_counter++;

   if ((millis() - m_last_fps_update) > 1000)
   {
      m_last_fps_update = millis();
      m_fps = m_fps_counter;
      m_fps_counter = 0;
   }
}


int Camera::getFps()
{
   return m_fps;
}
