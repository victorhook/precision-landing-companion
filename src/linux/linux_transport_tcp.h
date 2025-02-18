#ifndef TRANSPORT_TCP_LINUX
#define TRANSPORT_TCP_LINUX

#include "transport_tcp.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <string>

class TransportTCP_Linux : public TransportTCP
{
public:
    ~TransportTCP_Linux();
    using TransportTCP::TransportTCP;

    void init() override;
    bool readByte(uint8_t *byte, uint32_t timeoutMs) override;
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
    uint32_t writeBytes(const uint8_t *data, uint32_t len) override;


private:
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
