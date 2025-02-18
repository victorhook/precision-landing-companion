#include "hal_linux.h"

#include <chrono>
#include <thread>

// Delay for a specified number of milliseconds
void delay(const uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Delay for a specified number of microseconds
void delayMicroseconds(const uint32_t us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// Get the current time in milliseconds since program start
uint32_t millis()
{
    static auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}

// Get the current time in microseconds since program start
uint32_t micros()
{
    static auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
}
