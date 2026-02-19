#include "text_console.h"
#include "text_buffer.h"

#include "display.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "shell.h"

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

static text_buffer_t s_buf;
static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_render_task;
static bool s_initialized = false;

static FILE *s_original_stdout;

/* ── Rendering ─────────────────────────────────────────────── */

static void render_dirty(void)
{
    if (!text_buffer_has_dirty(&s_buf))
    {
        return;
    }

    char chars[TEXT_BUF_MAX_COLS];
    uint16_t fg[TEXT_BUF_MAX_COLS];
    uint64_t dirty = s_buf.dirty_rows;

    while (dirty)
    {
        int r = __builtin_ctzll(dirty);
        for (int c = 0; c < s_buf.cols; c++)
        {
            chars[c] = s_buf.cells[r][c].ch;
            fg[c] = s_buf.cells[r][c].fg;
        }
        display_draw_text_row(r * FONT_HEIGHT, chars, fg, s_buf.cols, BG_COLOR);
        text_buffer_clear_row_dirty(&s_buf, r);
        dirty &= dirty - 1;
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
            render_dirty();
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

    if (s_initialized && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        text_buffer_write(&s_buf, buf, size);
        xSemaphoreGive(s_mutex);
        xTaskNotifyGive(s_render_task);
    }

    return (ssize_t)written;
}

/* ── Line editor (keyboard input) ──────────────────────────── */

#define INPUT_LINE_MAX 128

static char s_input_line[INPUT_LINE_MAX];
static int s_input_pos;

static void input_print_prompt(void)
{
    const char *prompt = shell_get_prompt();
    if (prompt != NULL)
    {
        printf("%s", prompt);
    }
}

static void input_execute_line(void)
{
    s_input_line[s_input_pos] = '\0';
    printf("\n");

    if (s_input_pos > 0)
    {
        int ret = 0;
        esp_err_t err = esp_console_run(s_input_line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unknown command: %s\n", s_input_line);
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            /* Empty or whitespace-only command */
        }
        else if (err != ESP_OK)
        {
            printf("Error: %s\n", esp_err_to_name(err));
        }
    }

    s_input_pos = 0;
    input_print_prompt();
}

static void input_handle_char(char ch)
{
    if (ch == '\n' || ch == '\r')
    {
        input_execute_line();
        return;
    }

    if (ch == '\b' || ch == 0x7F)
    {
        if (s_input_pos > 0)
        {
            s_input_pos--;
            printf("\b \b");
        }
        return;
    }

    /* Ignore non-printable control characters */
    if (ch < 0x20 && ch != '\t')
    {
        return;
    }

    if (s_input_pos < INPUT_LINE_MAX - 1)
    {
        s_input_line[s_input_pos++] = ch;
        printf("%c", ch);
    }
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

    if (s_render_task != NULL)
    {
        vTaskDelete(s_render_task);
        s_render_task = NULL;
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
        xSemaphoreGive(s_mutex);
        xTaskNotifyGive(s_render_task);
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

extern "C" void text_console_input(const char *data, size_t len)
{
    if (!s_initialized || data == NULL || len == 0)
    {
        return;
    }

    for (size_t i = 0; i < len; i++)
    {
        input_handle_char(data[i]);
    }
}
