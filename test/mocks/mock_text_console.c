#include "text_console.h"

static unsigned s_clear_count;
static text_console_log_level_t s_log_level = TEXT_CONSOLE_LOG_WARN;

unsigned mock_text_console_clear_count(void)
{
    return s_clear_count;
}

void mock_text_console_reset(void)
{
    s_clear_count = 0;
    s_log_level = TEXT_CONSOLE_LOG_WARN;
}

esp_err_t text_console_init(void)
{
    return ESP_OK;
}

esp_err_t text_console_set_visible(bool visible)
{
    (void)visible;
    return ESP_OK;
}

void text_console_deinit(void) {}

void text_console_write(const char *data, size_t len)
{
    (void)data;
    (void)len;
}

void text_console_clear(void)
{
    s_clear_count++;
}

esp_err_t text_console_set_log_level(text_console_log_level_t level)
{
    s_log_level = level;
    return ESP_OK;
}

text_console_log_level_t text_console_get_log_level(void)
{
    return s_log_level;
}

const char *text_console_log_level_name(text_console_log_level_t level)
{
    switch (level)
    {
    case TEXT_CONSOLE_LOG_ERROR:
        return "error";
    case TEXT_CONSOLE_LOG_WARN:
        return "warn";
    case TEXT_CONSOLE_LOG_INFO:
        return "info";
    default:
        return "unknown";
    }
}

void text_console_resize(void) {}
