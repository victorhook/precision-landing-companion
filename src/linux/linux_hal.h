#ifndef LINUX_HAL_H
#define LINUX_HAL_H

// Include all hal drivers
#include "hal.h"
#include "linux_camera.h"
#include "linux_transport_udp.h"
#include "linux_transport_ap.h"
#include "linux_transport_tcp_server.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

/* Linux-specific thread-safe queue */
template <typename T>
class LinuxQueue : public HAL_Queue<T>
{
public:
    LinuxQueue(uint32_t size);
    ~LinuxQueue();

    bool push(const T &item, uint32_t timeoutMs = 0) override;
    bool pop(T &item, uint32_t timeoutMs = 0) override;
    bool isEmpty() override;
    bool isFull() override;
    uint32_t maxSize() override;

private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    uint32_t queueSize;
};

/* ✅ Move Template Implementation Here */
template <typename T>
LinuxQueue<T>::LinuxQueue(uint32_t size) : queueSize(size) {}

template <typename T>
LinuxQueue<T>::~LinuxQueue() {}

template <typename T>
bool LinuxQueue<T>::push(const T &item, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(mtx);
    if (timeoutMs > 0)
    {
        auto timeout = std::chrono::milliseconds(timeoutMs);
        if (!cv.wait_for(lock, timeout, [this] { return queue.size() < queueSize; }))
        {
            return false; // Timeout
        }
    }
    else if (queue.size() >= queueSize)
    {
        return false; // Queue is full
    }
    queue.push(item);
    cv.notify_one();
    return true;
}

template <typename T>
bool LinuxQueue<T>::pop(T &item, uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(mtx);
    if (timeoutMs > 0)
    {
        auto timeout = std::chrono::milliseconds(timeoutMs);
        if (!cv.wait_for(lock, timeout, [this] { return !queue.empty(); }))
        {
            return false; // Timeout
        }
    }
    else if (queue.empty())
    {
        return false; // Queue is empty
    }
    item = queue.front();
    queue.pop();
    cv.notify_one();
    return true;
}

template <typename T>
bool LinuxQueue<T>::isEmpty()
{
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
}

template <typename T>
bool LinuxQueue<T>::isFull()
{
    std::lock_guard<std::mutex> lock(mtx);
    return queue.size() >= queueSize;
}

template <typename T>
uint32_t LinuxQueue<T>::maxSize()
{
    return queueSize;
}

#endif // LINUX_HAL_H
