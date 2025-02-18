#include "linux_transport_tcp.h"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>


TransportTCP_Linux::~TransportTCP_Linux()
{
    running = false;
    closeSocket();
    if (tx_thread.joinable()) tx_thread.join();
    if (rx_thread.joinable()) rx_thread.join();
}

void TransportTCP_Linux::init()
{
    running = true;
    
    // Attempt to connect
    if (!connectToServer())
    {
        std::cerr << "Failed to connect to ArduPilot TCP server" << std::endl;
        return;
    }

    // Start TX and RX threads
    tx_thread = std::thread(&TransportTCP_Linux::txLoop, this);
    rx_thread = std::thread(&TransportTCP_Linux::rxLoop, this);
}

bool TransportTCP_Linux::connectToServer()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address" << std::endl;
        return false;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection to server failed" << std::endl;
        closeSocket();
        return false;
    }

    std::cout << "Connected to ArduPilot TCP server!" << std::endl;
    return true;
}

void TransportTCP_Linux::closeSocket()
{
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void TransportTCP_Linux::txLoop()
{
    while (running)
    {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(tx_mutex);
            tx_cv.wait(lock, [this] { return !tx_queue.empty() || !running; });

            if (!running)
                return;

            data = std::move(tx_queue.front());
            tx_queue.pop();
        }

        if (send(sockfd, data.data(), data.size(), 0) < 0)
        {
            std::cerr << "Send failed, reconnecting..." << std::endl;
            closeSocket();
            while (running && !connectToServer()) // Reconnect loop
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

void TransportTCP_Linux::rxLoop()
{
    uint8_t buffer[1024];

    while (running)
    {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            std::cerr << "Connection lost, attempting to reconnect..." << std::endl;
            closeSocket();
            while (running && !connectToServer())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(rx_mutex);
            rx_queue.emplace(buffer, buffer + bytes_received);
            rx_cv.notify_one(); // Wake up `readByteBlocking()`
        }
    }
}


uint32_t TransportTCP_Linux::writeBytes(const uint8_t *data, uint32_t len)
{
    if (!data || len == 0) return 0;

    std::lock_guard<std::mutex> lock(tx_mutex);
    tx_queue.push(std::vector<uint8_t>(data, data + len)); // Store as vector internally
    tx_cv.notify_one();
    return len;
}


uint32_t TransportTCP_Linux::readBytes(uint8_t *buffer, uint32_t max_len, uint32_t timeoutMs)
{
    if (!buffer || max_len == 0) return 0; // Fix: Return 0 instead of false

    std::unique_lock<std::mutex> lock(rx_mutex);

    // Wait for data to be available or timeout
    if (!rx_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return !rx_queue.empty(); }))
    {
        return 0; // Timeout, return 0 bytes read
    }

    // Get the front packet
    std::vector<uint8_t> data = std::move(rx_queue.front());
    rx_queue.pop();

    uint32_t bytesRead = std::min(static_cast<uint32_t>(data.size()), max_len);
    std::memcpy(buffer, data.data(), bytesRead);

    return bytesRead; // Return the actual number of bytes read
}



bool TransportTCP_Linux::readByte(uint8_t *byte, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(rx_mutex);
    
    // Wait for data to be available or timeout
    if (!rx_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return !rx_queue.empty(); }))
    {
        return false; // Timeout
    }

    // Extract the first byte from the front of the queue
    if (!rx_queue.empty())
    {
        std::vector<uint8_t> &front = rx_queue.front();
        *byte = front.front();
        front.erase(front.begin()); // Remove the read byte

        // If the vector is empty after reading, remove it from the queue
        if (front.empty())
        {
            rx_queue.pop();
        }

        return true; // Successfully read a byte
    }

    return false; // Should not happen, but ensures safety
}
