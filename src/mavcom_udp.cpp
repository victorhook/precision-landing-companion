#include "mavcom_udp.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

MavComUDP::MavComUDP(const std::string &ip, int port) 
    : server_ip(ip), server_port(port), sockfd(-1), running(false) {}

MavComUDP::~MavComUDP()
{
    running = false;
    closeSocket();
    if (rx_thread.joinable()) rx_thread.join();
}

void MavComUDP::init()
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating UDP socket" << std::endl;
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    running = true;
    rx_thread = std::thread(&MavComUDP::rxLoop, this);
}

void MavComUDP::closeSocket()
{
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void MavComUDP::rxLoop()
{
    uint8_t buffer[1024];

    while (running)
    {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (bytes_received > 0)
        {
            std::lock_guard<std::mutex> lock(rx_mutex);
            for (ssize_t i = 0; i < bytes_received; ++i)
            {
                rx_queue.push(buffer[i]);
            }
            rx_cv.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Avoid CPU overload
    }
}

bool MavComUDP::readByte(uint8_t *byte, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(rx_mutex);
    
    if (!rx_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return !rx_queue.empty(); }))
    {
        return false; // Timeout
    }

    *byte = rx_queue.front();
    rx_queue.pop();
    return true;
}

uint32_t MavComUDP::readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(rx_mutex);
    
    if (!rx_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return !rx_queue.empty(); }))
    {
        return 0; // Timeout
    }

    uint32_t bytesRead = 0;
    while (!rx_queue.empty() && bytesRead < maxLen)
    {
        bytes[bytesRead++] = rx_queue.front();
        rx_queue.pop();
    }

    return bytesRead;
}

uint32_t MavComUDP::writeBytes(const uint8_t *data, uint32_t len)
{
    ssize_t bytes_sent = sendto(sockfd, data, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    return (bytes_sent > 0) ? bytes_sent : 0;
}
