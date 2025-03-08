#include "globals.h"
#include "telemetry.h"

extern Telemetry telemetry;
extern uint32_t frame;

bool telemetryClientIsConnected()
{
    return telemetry.clientConnected();
}


bool getTelemetryClientIp(char ip[17])
{
    return telemetry.getClientIP(ip);
}

uint32_t get_frame_nbr()
{
    return frame;
}