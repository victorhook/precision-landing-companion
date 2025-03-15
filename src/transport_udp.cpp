#include "transport_udp.h"
#include <string.h>
#include "log.h"

TransportUDP::TransportUDP(const int port)
: port(port), initialized(false)
{

}

void TransportUDP::init(const char* remoteIP)
{
    strncpy(this->remoteIP, remoteIP, 16);
    this->remoteIP[16] = '\0';
    debug("UDP server starting at %s:%d", this->remoteIP, port);
    doInit();
    initialized = true;
}

void TransportUDP::deInit()
{
    doDeInit();
    initialized = false;
}

bool TransportUDP::isInitialized()
{
    return initialized;
}
