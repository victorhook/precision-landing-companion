#ifndef TRANSPORT_UDP_LINUX_H
#define TRANSPORT_UDP_LINUX_H

#include "transport_udp.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <vector>
#include <string>
#include <netinet/in.h>

class TransportUDP_Linux : public TransportUDP
{
public:
    using TransportUDP::TransportUDP;
    ~TransportUDP_Linux();
    void doInit() override;
    void doDeInit() override;
    bool readByte(uint8_t *byte, uint32_t timeoutMs) override;
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
    uint32_t writeBytes(const uint8_t *data, uint32_t len) override;

private:
    int server_port;
    int sockfd;

    std::thread rx_thread;
    std::atomic<bool> running;

    std::mutex rx_mutex;
    std::condition_variable rx_cv;
    std::queue<uint8_t> rx_queue;

    struct sockaddr_in server_addr;

    void rxLoop();
    void closeSocket();
};

#endif // TRANSPORT_UDP_LINUX_H
