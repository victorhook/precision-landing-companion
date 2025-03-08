#ifndef TRANSPORT_TCP_LINUX
#define TRANSPORT_TCP_LINUX

#include "transport_ap.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <string>

class TransportAP_Linux : public TransportAP
{
public:
    TransportAP_Linux(const char* ip = "127.0.0.1", const uint16_t port = 5760);
    ~TransportAP_Linux();
    using TransportAP::TransportAP;

    bool init() override;
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
    uint32_t writeBytes(const uint8_t *data, uint32_t len) override;
    bool readByte(uint8_t *byte, uint32_t timeoutMs);

private:
    const char* ip;
    const uint16_t port;
    int sockfd;

    std::condition_variable rx_cv;
    std::thread tx_thread;
    std::thread rx_thread;
    std::atomic<bool> running;

    std::mutex tx_mutex;
    std::mutex rx_mutex;
    std::condition_variable tx_cv;

    std::queue<std::vector<uint8_t>> tx_queue;
    std::queue<std::vector<uint8_t>> rx_queue;

    void txLoop();
    void rxLoop();
    bool connectToServer();
    void closeSocket();

};

#endif // TRANSPORT_TCP_LINUX
