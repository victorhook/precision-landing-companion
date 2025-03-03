#include "globals.h"
#include "telemetry.h"

extern Telemetry telemetry;


bool telemetryClientIsConnected()
{
    return telemetry.clientConnected();
}


bool getTelemetryClientIp(char ip[17])
{
    return telemetry.getClientIP(ip);
}
