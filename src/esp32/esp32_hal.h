#ifndef ESP32_HAL_H
#define ESP32_HAL_H

#include "hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* ESP32-specific thread-safe queue */
template <typename T>
class ESP32Queue : public HAL_Queue<T>
{
public:
    ESP32Queue(uint32_t size);
    ~ESP32Queue();

    bool push(const T &item, uint32_t timeoutMs = 0) override;
    bool pop(T &item, uint32_t timeoutMs = 0) override;
    bool isEmpty() override;
    bool isFull() override;
    uint32_t maxSize() override;

private:
    QueueHandle_t queueHandle;
    uint32_t queueSize;
};

/* ✅ Move Template Implementation Here */
template <typename T>
ESP32Queue<T>::ESP32Queue(uint32_t size)
{
    queueSize = size;
    queueHandle = xQueueCreate(size, sizeof(T));  // ✅ Create FreeRTOS queue
}

template <typename T>
ESP32Queue<T>::~ESP32Queue()
{
    vQueueDelete(queueHandle);  // ✅ Delete queue when object is destroyed
}

template <typename T>
bool ESP32Queue<T>::push(const T &item, uint32_t timeoutMs)
{
    TickType_t ticks = timeoutMs == 0 ? 0 : pdMS_TO_TICKS(timeoutMs);
    return (xQueueSend(queueHandle, &item, ticks) == pdPASS);
}

template <typename T>
bool ESP32Queue<T>::pop(T &item, uint32_t timeoutMs)
{
    TickType_t ticks = timeoutMs == 0 ? 0 : pdMS_TO_TICKS(timeoutMs);
    return (xQueueReceive(queueHandle, &item, ticks) == pdPASS);
}

template <typename T>
bool ESP32Queue<T>::isEmpty()
{
    return (uxQueueMessagesWaiting(queueHandle) == 0);
}

template <typename T>
bool ESP32Queue<T>::isFull()
{
    return (uxQueueMessagesWaiting(queueHandle) == queueSize);
}

template <typename T>
uint32_t ESP32Queue<T>::maxSize()
{
    return queueSize;
}


#endif // ESP32_HAL_H
