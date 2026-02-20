#include "text_console.h"

esp_err_t text_console_init(void)
{
    return ESP_OK;
}

void text_console_deinit(void) {}

void text_console_write(const char *data, size_t len)
{
    (void)data;
    (void)len;
}

void text_console_clear(void) {}

void text_console_resize(void) {}

void text_console_register_commands(void) {}
