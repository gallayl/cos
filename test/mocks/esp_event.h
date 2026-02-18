#pragma once

#include "esp_err.h"

typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,
                                    void *event_data);

#define ESP_EVENT_ANY_ID (-1)

extern esp_event_base_t WIFI_EVENT;
extern esp_event_base_t IP_EVENT;

#define WIFI_EVENT_STA_CONNECTED  4
#define WIFI_EVENT_STA_DISCONNECTED 5
#define IP_EVENT_STA_GOT_IP 0

esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id,
                                              esp_event_handler_t event_handler, void *event_handler_arg,
                                              esp_event_handler_instance_t *instance);
