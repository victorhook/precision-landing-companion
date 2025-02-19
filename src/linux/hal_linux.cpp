#include "hal.h"

#include <chrono>
#include <thread>

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

int main()
{
    setup();

    while (1)
    {
        loop();
    }

    return 0;
}
