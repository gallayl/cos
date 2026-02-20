#pragma once

#include "freertos/FreeRTOS.h"

typedef void *TaskHandle_t;

#define pdPASS 1

#define portMAX_DELAY ((TickType_t)0xFFFFFFFF)

BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName, unsigned int usStackDepth, void *pvParameters,
                       unsigned int uxPriority, TaskHandle_t *pxCreatedTask);

void vTaskDelete(TaskHandle_t xTask);
