#include "mock_esp_event.h"

#include <string.h>

esp_event_base_t WIFI_EVENT = "WIFI_EVENT";
esp_event_base_t IP_EVENT = "IP_EVENT";

#define MOCK_MAX_HANDLERS 8

typedef struct
{
    esp_event_base_t base;
    int32_t event_id;
    esp_event_handler_t handler;
    void *arg;
    int used;
} mock_handler_t;

static mock_handler_t s_handlers[MOCK_MAX_HANDLERS];
static int s_handler_count = 0;

void mock_esp_event_reset(void)
{
    memset(s_handlers, 0, sizeof(s_handlers));
    s_handler_count = 0;
}

void mock_esp_event_fire(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    for (int i = 0; i < s_handler_count; i++)
    {
        mock_handler_t *h = &s_handlers[i];
        if (h->used && h->base == event_base && (h->event_id == ESP_EVENT_ANY_ID || h->event_id == event_id))
        {
            h->handler(h->arg, event_base, event_id, event_data);
        }
    }
}

esp_err_t esp_event_loop_create_default(void)
{
    return ESP_OK;
}

esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id,
                                              esp_event_handler_t event_handler, void *event_handler_arg,
                                              esp_event_handler_instance_t *instance)
{
    if (s_handler_count >= MOCK_MAX_HANDLERS)
    {
        return ESP_ERR_NO_MEM;
    }

    mock_handler_t *h = &s_handlers[s_handler_count];
    h->base = event_base;
    h->event_id = event_id;
    h->handler = event_handler;
    h->arg = event_handler_arg;
    h->used = 1;
    s_handler_count++;

    if (instance != NULL)
    {
        *instance = (esp_event_handler_instance_t)h;
    }
    return ESP_OK;
}
