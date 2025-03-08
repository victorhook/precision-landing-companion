#include "linux_hal.h"

#include <chrono>
#include <thread>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdint>
#include <iostream> 
#include <fstream>
#include <signal.h>

void handle_sigint(int signal) {
    std::cout << "Caught SIGINT (Ctrl+C), exiting gracefully..." << std::endl;
    exit(0);
}

void hal_init()
{
    signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE
    signal(SIGINT, handle_sigint);  // Handle Ctrl+C
}

void hal_delay(const uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void hal_delayMicroseconds(const uint32_t us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

uint32_t hal_millis()
{
    static auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}

uint32_t hal_micros()
{
    static auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
}

void hal_get_ip(char ip[17])
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            const char *addrStr = inet_ntop(AF_INET, &addr->sin_addr, ip, 17);
            if (addrStr) {
                break;  // Stop at the first valid IPv4 address
            }
        }
    }

    freeifaddrs(ifaddr);
}

uint32_t hal_get_free_heap()
{
    std::ifstream meminfo("/proc/meminfo"); // Ensure correct inclusion
    if (!meminfo.is_open()) {
        std::cerr << "Error: Unable to open /proc/meminfo" << std::endl;
        return 0;
    }

    std::string line;
    uint32_t freeMem = 0;

    while (std::getline(meminfo, line)) {
        if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %u kB", &freeMem);
            return freeMem * 1024;  // Convert from KB to bytes
        }
    }
    return 0;
}

void setup();
void loop();

int main()
{
    setup();

    while (1)
    {
        loop();
    }

    return 0;
}
