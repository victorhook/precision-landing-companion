#ifndef LINUX_TRANSPORT_TCP_SERVER_H
#define LINUX_TRANSPORT_TCP_SERVER_H

#include "transport_tcp_server.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <netinet/in.h>

class TransportTCP_Server_Linux : public TransportTCP_Server
{
public:
    TransportTCP_Server_Linux(int port);
    ~TransportTCP_Server_Linux();

    void init() override;
    bool readByte(uint8_t *byte, uint32_t timeoutMs) override;
    uint32_t readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs) override;
    uint32_t writeBytes(const uint8_t *data, uint32_t len) override;
    void flushTX() override;
    void disconnectClient() override;
    bool clientConnected() override;
    bool getClientIp(char ip[17]) override;

private:
    void serverThread();
    int serverSocket;
    int clientSocket;
    std::thread serverThreadHandle;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> isRunning;
};

#endif // LINUX_TRANSPORT_TCP_SERVER_H