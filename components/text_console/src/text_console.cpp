#include "text_console.h"
#include "text_buffer.h"

#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "text_console";

#define FONT_ID 1
#define FONT_WIDTH 6
#define FONT_HEIGHT 8
#define BG_COLOR TEXT_BUF_COLOR_BLACK

static text_buffer_t s_buf;
static SemaphoreHandle_t s_mutex;
static bool s_initialized = false;

static FILE *s_original_stdout;

/* ── Rendering ─────────────────────────────────────────────── */

static void render_dirty(void)
{
    if (!text_buffer_has_dirty(&s_buf))
    {
        return;
    }

    display_start_write();
    for (int r = 0; r < s_buf.rows; r++)
    {
        for (int c = 0; c < s_buf.cols; c++)
        {
            text_cell_t *cell = &s_buf.cells[r][c];
            if (!cell->dirty)
            {
                continue;
            }
            int px = c * FONT_WIDTH;
            int py = r * FONT_HEIGHT;
            display_draw_char(px, py, cell->ch, cell->fg, BG_COLOR);
            cell->dirty = false;
        }
    }
    display_end_write();
}

/* ── stdout hook ───────────────────────────────────────────── */

static ssize_t stdout_write_hook(void *cookie, const char *buf, size_t size)
{
    (void)cookie;

    /* Forward to original UART stdout */
    size_t written = fwrite(buf, 1, size, s_original_stdout);
    fflush(s_original_stdout);

    if (s_initialized && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        text_buffer_write(&s_buf, buf, size);
        render_dirty();
        xSemaphoreGive(s_mutex);
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

    int cols = display_get_width() / FONT_WIDTH;
    int rows = display_get_height() / FONT_HEIGHT;
    text_buffer_init(&s_buf, cols, rows);

    display_fill_screen(BG_COLOR);

    /* Hook stdout via fopencookie -- captures both printf() and ESP_LOG output
       since ESP_LOG's default vprintf handler writes to stdout. */
    s_original_stdout = stdout;
    cookie_io_functions_t fns = {};
    fns.write = stdout_write_hook;
    FILE *tee = fopencookie(NULL, "w", fns);
    if (tee != NULL)
    {
        setvbuf(tee, NULL, _IOLBF, 0);
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
        stdout = s_original_stdout;
        fclose(tee);
    }

    if (s_mutex != NULL)
    {
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
        text_buffer_write(&s_buf, data, len);
        render_dirty();
        xSemaphoreGive(s_mutex);
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
        render_dirty();
        xSemaphoreGive(s_mutex);
    }
}
