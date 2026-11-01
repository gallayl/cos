#include "shell.h"
#include "shell_input.h"

esp_err_t shell_input_init(void)
{
    return ESP_OK;
}

void shell_feed_input(const char *data, size_t len)
{
    (void)data;
    (void)len;
}

esp_err_t shell_input_read_line(const char *prompt, shell_line_input_callback_t callback, void *context)
{
    (void)prompt;
    (void)callback;
    (void)context;
    return ESP_ERR_INVALID_STATE;
}

esp_err_t shell_input_cancel_line(void *context)
{
    (void)context;
    return ESP_ERR_NOT_FOUND;
}

bool shell_input_is_reading_line(void)
{
    return false;
}
