#include "linux_transport_tcp_server.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "log.h"

TransportTCP_Server_Linux::TransportTCP_Server_Linux(int port)
: TransportTCP_Server(port), serverSocket(-1), clientSocket(-1), isRunning(false)
{
    
}

TransportTCP_Server_Linux::~TransportTCP_Server_Linux()
{
    isRunning = false;
    if (serverThreadHandle.joinable())
        serverThreadHandle.join();
    if (clientSocket != -1)
        close(clientSocket);
    if (serverSocket != -1)
        close(serverSocket);
}

void TransportTCP_Server_Linux::init()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        error("Error: Unable to create socket\n");
        return;
    }
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        error("Error: Failed to set SO_REUSEADDR\n");
    }
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        error("Error: Failed to set SO_REUSEPORT\n");
    }


    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Error: Bind failed\n");
        return;
    }

    if (listen(serverSocket, 1) < 0)
    {
        printf("Error: Listen failed\n");
        return;
    }

    isRunning = true;
    serverThreadHandle = std::thread(&TransportTCP_Server_Linux::serverThread, this);
}

void TransportTCP_Server_Linux::serverThread()
{
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    while (1)
    {
        info("Waiting for client...\n");
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            printf("Error: Accept failed\n");
            return;
        }
        info("New client connected\n");
    }

}

bool TransportTCP_Server_Linux::readByte(uint8_t *byte, uint32_t timeoutMs)
{
    if (clientSocket == -1)
        return false;
    
    return recv(clientSocket, byte, 1, 0) > 0;
}

uint32_t TransportTCP_Server_Linux::readBytes(uint8_t *bytes, uint32_t maxLen, uint32_t timeoutMs)
{
    if (clientSocket == -1)
        return 0;
    
    ssize_t received = recv(clientSocket, bytes, maxLen, MSG_DONTWAIT);
    
    if (received < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return 0; // Timeout occurred
        }
        error("Error: recv() failed, errno = %d (%s)\n", errno, strerror(errno));
        return 0;
    }
    return static_cast<uint32_t>(received);
}

uint32_t TransportTCP_Server_Linux::writeBytes(const uint8_t *data, uint32_t len)
{
    if (clientSocket == -1)
        return 0;

    ssize_t sentBytes = send(clientSocket, data, len, 0);
    if (sentBytes == -1)
    {
        int err = errno;
        error("Error: send() failed, errno = %d (%s)\n", err, strerror(err));
        
        if (err == EPIPE || err == ECONNRESET)
        {
            warning("Client disconnected, closing socket...\n");
            disconnectClient();
        }
        return 0;
    }
    
    return static_cast<uint32_t>(sentBytes);
}


void TransportTCP_Server_Linux::flushTX()
{
    // No explicit flush needed for TCP
}

void TransportTCP_Server_Linux::disconnectClient()
{
    if (clientSocket != -1)
    {
        close(clientSocket);
        clientSocket = -1;
    }
}

bool TransportTCP_Server_Linux::clientConnected()
{
    return clientSocket != -1;
}

bool TransportTCP_Server_Linux::getClientIp(char ip[17])
{
    if (clientSocket == -1)
        return false;
    
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getpeername(clientSocket, (struct sockaddr *)&addr, &len) == 0)
    {
        inet_ntop(AF_INET, &addr.sin_addr, ip, 17);
        return true;
    }
    return false;
}