#ifndef MAVCOM_UDP_H
#define MAVCOM_UDP_H

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <vector>
#include <string>
#include <netinet/in.h>

class MavComUDP
{
public:
    MavComUDP(const std::string &ip, int port);
    ~MavComUDP();

    void init();
    bool readByte(uint8_t *byte, uint32_t timeoutMs);
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs);
    uint32_t writeBytes(const uint8_t *data, uint32_t len);

private:
    std::string server_ip;
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

#endif // MAVCOM_UDP_H
