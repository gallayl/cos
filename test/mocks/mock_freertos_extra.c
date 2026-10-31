#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include <string.h>

static int s_dummy_stream;
static int s_dummy_task;

StreamBufferHandle_t xStreamBufferCreate(size_t xBufferSizeBytes, size_t xTriggerLevelBytes)
{
    (void)xBufferSizeBytes;
    (void)xTriggerLevelBytes;
    return (StreamBufferHandle_t)&s_dummy_stream;
}

size_t xStreamBufferSend(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes,
                         TickType_t xTicksToWait)
{
    (void)xStreamBuffer;
    (void)pvTxData;
    (void)xTicksToWait;
    return xDataLengthBytes;
}

size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer, void *pvRxData, size_t xBufferLengthBytes,
                            TickType_t xTicksToWait)
{
    (void)xStreamBuffer;
    (void)pvRxData;
    (void)xBufferLengthBytes;
    (void)xTicksToWait;
    return 0;
}

BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName, unsigned int usStackDepth, void *pvParameters,
                       unsigned int uxPriority, TaskHandle_t *pxCreatedTask)
{
    (void)pxTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    if (pxCreatedTask != NULL)
    {
        *pxCreatedTask = (TaskHandle_t)&s_dummy_task;
    }
    return pdPASS;
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return (TaskHandle_t)&s_dummy_task;
}

void vTaskDelete(TaskHandle_t xTask)
{
    (void)xTask;
}
