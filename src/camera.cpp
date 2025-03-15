#include "camera.h"

#include "hal.h"
#include "globals.h"
#include "log.h"


Camera::Camera(TargetDetector* targetDetector)
: targetDetector(targetDetector), m_isInitialized(0), m_last_fps_update(0), m_fps(0), m_fps_counter(0), m_thrown_frames(0)
{
   udp = new TRANSPORT_UDP_CLASS(9095);
}

bool Camera::init()
{
   m_isInitialized = doInit();

   return m_isInitialized;
}

camera_meta_data_t Camera::getMetaData()
{
   return
   {
      .img_width = 320,
      .img_height = 240,
      .fov = 66
   };
}


void Camera::capture()
{
   if (!m_isInitialized)
   {
      return;
   }

   initUdpBroadcastIfNeeded();

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

void Camera::initUdpBroadcastIfNeeded()
{
   if (!udp->isInitialized() && telemetryClientIsConnected())
   {
      char ip[17];
      getTelemetryClientIp(ip);
      udp->init(ip);
      info("Starting camera broadcast to %s\n", ip);
   }
}
