#pragma once

#include "esp_err.h"

typedef enum
{
    ESP_SNTP_OPMODE_POLL = 0,
} esp_sntp_operatingmode_t;

void esp_sntp_setoperatingmode(esp_sntp_operatingmode_t operating_mode);
void esp_sntp_setservername(int index, const char *server);
void esp_sntp_init(void);
