#include "text_console.h"
#include "text_buffer.h"

#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *const TAG = "text_console";

#define FONT_ID 1
#define FONT_WIDTH 6
#define FONT_HEIGHT 8
#define BG_COLOR TEXT_BUF_COLOR_BLACK
#define RENDER_TASK_STACK 3072
#define RENDER_TASK_PRIO 1
#define RENDER_PERIOD_MS 30

#define TEXT_CONSOLE_NVS_NAMESPACE "console"
#define TEXT_CONSOLE_NVS_KEY_LOG_LEVEL "log_level"

static text_buffer_t s_buf;
static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_render_task;
static bool s_initialized = false;
static bool s_screen_visible = true;
static bool s_touch_active = false;
static int s_touch_last_y = 0;
static int s_touch_remainder = 0;

static FILE *s_original_stdout;
static text_console_log_level_t s_log_level = TEXT_CONSOLE_LOG_WARN;

static esp_log_level_t esp_log_level(text_console_log_level_t level)
{
    switch (level)
    {
    case TEXT_CONSOLE_LOG_ERROR:
        return ESP_LOG_ERROR;
    case TEXT_CONSOLE_LOG_INFO:
        return ESP_LOG_INFO;
    case TEXT_CONSOLE_LOG_WARN:
    default:
        return ESP_LOG_WARN;
    }
}

static bool valid_log_level(uint8_t level)
{
    return level <= TEXT_CONSOLE_LOG_INFO;
}

static void restore_log_level(void)
{
    s_log_level = TEXT_CONSOLE_LOG_WARN;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(TEXT_CONSOLE_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK)
    {
        uint8_t stored_level = TEXT_CONSOLE_LOG_WARN;
        err = nvs_get_u8(handle, TEXT_CONSOLE_NVS_KEY_LOG_LEVEL, &stored_level);
        nvs_close(handle);
        if (err == ESP_OK)
        {
            if (valid_log_level(stored_level))
            {
                s_log_level = static_cast<text_console_log_level_t>(stored_level);
            }
            else
            {
                ESP_LOGW(TAG, "Ignoring invalid stored log level %u", static_cast<unsigned>(stored_level));
            }
        }
        else if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "Could not read log level from NVS: %s", esp_err_to_name(err));
        }
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Could not open console settings: %s", esp_err_to_name(err));
    }

    esp_log_level_set("*", esp_log_level(s_log_level));
}

/* ── Rendering ─────────────────────────────────────────────── */

static void render_dirty(void)
{
    if (!text_buffer_has_dirty(&s_buf))
    {
        return;
    }

    uint64_t dirty = s_buf.dirty_rows;

    while (dirty)
    {
        int r = __builtin_ctzll(dirty);
        char chars[TEXT_BUF_MAX_COLS];
        uint16_t fg[TEXT_BUF_MAX_COLS];
        if (text_buffer_get_visible_row(&s_buf, r, chars, fg))
        {
            display_draw_text_row(r * FONT_HEIGHT, chars, fg, s_buf.cols, BG_COLOR);
            text_buffer_clear_row_dirty(&s_buf, r);
        }
        dirty &= dirty - 1;
    }
}

static void handle_console_touch(void)
{
    int x;
    int y;
    if (display_get_touch(&x, &y))
    {
        (void)x;
        if (!s_touch_active)
        {
            s_touch_active = true;
            s_touch_last_y = y;
            s_touch_remainder = 0;
            return;
        }

        const int movement = y - s_touch_last_y + s_touch_remainder;
        const int rows = movement / FONT_HEIGHT;
        s_touch_last_y = y;
        s_touch_remainder = movement - rows * FONT_HEIGHT;
        if (rows != 0)
        {
            (void)text_buffer_scroll_view(&s_buf, rows);
        }
    }
    else
    {
        s_touch_active = false;
        s_touch_remainder = 0;
    }
}

static void render_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(RENDER_PERIOD_MS));

        if (!s_initialized)
        {
            continue;
        }

        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            if (s_screen_visible)
            {
                handle_console_touch();
                render_dirty();
            }
            else
            {
                s_touch_active = false;
                s_touch_remainder = 0;
            }
            xSemaphoreGive(s_mutex);
        }
    }
}

/* ── stdout hook ───────────────────────────────────────────── */

static ssize_t stdout_write_hook(void *cookie, const char *buf, size_t size)
{
    (void)cookie;

    /* Forward to original UART stdout */
    size_t written = fwrite(buf, 1, size, s_original_stdout);
    fflush(s_original_stdout);

    bool history_allocation_failed = false;
    bool should_notify_render = false;
    if (s_initialized && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        if (s_screen_visible)
        {
            text_buffer_write(&s_buf, buf, size);
            history_allocation_failed = text_buffer_take_scrollback_allocation_failure(&s_buf);
            should_notify_render = true;
        }
        xSemaphoreGive(s_mutex);
        if (should_notify_render)
        {
            xTaskNotifyGive(s_render_task);
        }
    }
    if (history_allocation_failed)
    {
        ESP_LOGW(TAG, "Scrollback allocation failed; continuing without history until the console is cleared");
    }

    return (ssize_t)written;
}

/* ── Public API ────────────────────────────────────────────── */

extern "C" esp_err_t text_console_init(void)
{
    if (s_initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    display_set_text_font(FONT_ID);
    s_screen_visible = true;
    restore_log_level();

    int cols = display_get_width() / FONT_WIDTH;
    int rows = display_get_height() / FONT_HEIGHT;
    text_buffer_init(&s_buf, cols, rows);

    display_fill_screen(BG_COLOR);

    BaseType_t rc = xTaskCreate(render_task, "tc_render", RENDER_TASK_STACK, NULL, RENDER_TASK_PRIO, &s_render_task);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create render task");
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* Hook stdout via fopencookie -- captures both printf() and ESP_LOG output
       since ESP_LOG's default vprintf handler writes to stdout. */
    s_original_stdout = stdout;
    cookie_io_functions_t fns = {};
    fns.write = stdout_write_hook;
    FILE *tee = fopencookie(NULL, "w", fns);
    if (tee != NULL)
    {
        setvbuf(tee, NULL, _IOLBF, 0);
        /* Both must be updated: `stdout` is the C global, but newlib's
           per-task _reent struct caches its own _stdout pointer. Tasks
           created before this point still reference the original fd. */
        stdout = tee;
    }
    else
    {
        ESP_LOGW(TAG, "fopencookie failed, stdout mirroring disabled");
    }

    s_initialized = true;

    text_console_register_commands();

    ESP_LOGI(TAG, "Text console ready (%dx%d chars)", cols, rows);
    return ESP_OK;
}

extern "C" esp_err_t text_console_set_visible(bool visible)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return ESP_OK;
    }

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Could not change console visibility");
        return ESP_ERR_TIMEOUT;
    }
    s_screen_visible = visible;
    if (!visible)
    {
        s_touch_active = false;
        s_touch_remainder = 0;
    }
    if (visible)
    {
        for (int row = 0; row < s_buf.rows; ++row)
        {
            for (int col = 0; col < s_buf.cols; ++col)
            {
                s_buf.cells[row][col].dirty = true;
            }
            s_buf.dirty_rows |= (1ULL << row);
        }
    }
    xSemaphoreGive(s_mutex);

    if (visible && s_render_task != NULL)
    {
        xTaskNotifyGive(s_render_task);
    }
    return ESP_OK;
}

extern "C" void text_console_deinit(void)
{
    if (!s_initialized)
    {
        return;
    }

    s_initialized = false;

    /* Restore stdout */
    if (stdout != s_original_stdout)
    {
        FILE *tee = stdout;
        /* Mirror the init path: restore both the C global and newlib's
           per-task _reent cache so all tasks pick up the change. */
        stdout = s_original_stdout;
        fclose(tee);
    }

    if (s_render_task != NULL)
    {
        vTaskDelete(s_render_task);
        s_render_task = NULL;
    }

    if (s_mutex != NULL)
    {
        text_buffer_clear(&s_buf);
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }

    ESP_LOGI(TAG, "Text console deinitialized");
}

extern "C" void text_console_write(const char *data, size_t len)
{
    if (!s_initialized || data == NULL || len == 0)
    {
        return;
    }

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        const bool screen_visible = s_screen_visible;
        if (screen_visible)
        {
            text_buffer_write(&s_buf, data, len);
        }
        const bool history_allocation_failed = screen_visible && text_buffer_take_scrollback_allocation_failure(&s_buf);
        xSemaphoreGive(s_mutex);
        if (screen_visible)
        {
            xTaskNotifyGive(s_render_task);
        }
        if (history_allocation_failed)
        {
            ESP_LOGW(TAG, "Scrollback allocation failed; continuing without history until the console is cleared");
        }
    }
}

extern "C" esp_err_t text_console_release_scrollback(void)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return ESP_OK;
    }
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    text_buffer_clear_scrollback(&s_buf);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

extern "C" esp_err_t text_console_set_log_level(text_console_log_level_t level)
{
    if (level != TEXT_CONSOLE_LOG_ERROR && level != TEXT_CONSOLE_LOG_WARN && level != TEXT_CONSOLE_LOG_INFO)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(TEXT_CONSOLE_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }
    err = nvs_set_u8(handle, TEXT_CONSOLE_NVS_KEY_LOG_LEVEL, static_cast<uint8_t>(level));
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    if (err != ESP_OK)
    {
        return err;
    }

    s_log_level = level;
    esp_log_level_set("*", esp_log_level(level));
    return ESP_OK;
}

extern "C" text_console_log_level_t text_console_get_log_level(void)
{
    return s_log_level;
}

extern "C" const char *text_console_log_level_name(text_console_log_level_t level)
{
    switch (level)
    {
    case TEXT_CONSOLE_LOG_ERROR:
        return "error";
    case TEXT_CONSOLE_LOG_INFO:
        return "info";
    case TEXT_CONSOLE_LOG_WARN:
        return "warn";
    default:
        return "unknown";
    }
}

extern "C" void text_console_clear(void)
{
    if (!s_initialized)
    {
        return;
    }

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        text_buffer_clear(&s_buf);
        display_fill_screen(BG_COLOR);
        xSemaphoreGive(s_mutex);
        xTaskNotifyGive(s_render_task);
    }
}

extern "C" void text_console_resize(void)
{
    if (!s_initialized)
    {
        return;
    }

    int cols = display_get_width() / FONT_WIDTH;
    int rows = display_get_height() / FONT_HEIGHT;

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        text_buffer_resize(&s_buf, cols, rows);
        display_fill_screen(BG_COLOR);
        xSemaphoreGive(s_mutex);
        xTaskNotifyGive(s_render_task);
    }

    ESP_LOGI(TAG, "Console resized to %dx%d chars", cols, rows);
}
