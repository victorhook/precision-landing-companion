#ifndef MAVCOM_TCP_H
#define MAVCOM_TCP_H

#include "mavcom.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <string>

class MavComTCP : public MavCom
{
public:
    MavComTCP(const std::string &ip, int port);
    ~MavComTCP();

    void doInit() override;

    void sendData(const uint8_t *data, uint32_t len) override;
    bool readByteBlocking(uint8_t &byte, uint32_t timeout_ms = 100) override;
    uint32_t readBytes(uint8_t *buffer, uint32_t len, uint32_t max_len) override;


private:
    std::string server_ip;
    int server_port;
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

#endif // MAVCOM_TCP_H
