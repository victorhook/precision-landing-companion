#include <esp32/esp32_transport_ap.h>


TransportAP_ESP32::TransportAP_ESP32()
{
    serial = &Serial1;
}

bool TransportAP_ESP32::init()
{
    serial->begin(AP_BAUDRATE);
    return serial->setPins(AP_UART_RX, AP_UART_TX);
}

uint32_t TransportAP_ESP32::readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs)
{
    return serial->readBytes(bytes, maxLen);
}

uint32_t TransportAP_ESP32::writeBytes(const uint8_t *data, uint32_t len)
{
    return serial->write(data, len);
}
