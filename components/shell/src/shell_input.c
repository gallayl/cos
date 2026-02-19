#include "shell_input.h"
#include "shell.h"

#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "shell_input";

#define INPUT_LINE_MAX 128
#define INPUT_STREAM_SIZE 256
#define HISTORY_SIZE 8
#define UART_RX_BUF_SIZE 256
#define UART_NUM CONFIG_ESP_CONSOLE_UART_NUM

#define RX_TASK_STACK 2048
#define RX_TASK_PRIO 5
#define INPUT_TASK_STACK 8192
#define INPUT_TASK_PRIO 4

static StreamBufferHandle_t s_input_stream;

/* ── Line buffer ───────────────────────────────────────────── */

static char s_line[INPUT_LINE_MAX];
static int s_line_pos;

/* ── Command history ───────────────────────────────────────── */

static char s_history[HISTORY_SIZE][INPUT_LINE_MAX];
static int s_history_count;
static int s_history_write;   /* next slot to write */
static int s_history_browse;  /* browse offset, -1 = live line */
static char s_saved_line[INPUT_LINE_MAX];
static int s_saved_pos;

/* ── ESC sequence parser state ─────────────────────────────── */

typedef enum
{
    ST_NORMAL,
    ST_ESC,
    ST_CSI,
} input_state_t;

static input_state_t s_parse_state;

/* ── Helpers ───────────────────────────────────────────────── */

static void erase_line_on_screen(void)
{
    for (int i = 0; i < s_line_pos; i++)
    {
        printf("\b \b");
    }
}

static void replace_line(const char *new_line)
{
    erase_line_on_screen();
    int new_len = (int)strlen(new_line);
    if (new_len >= INPUT_LINE_MAX)
    {
        new_len = INPUT_LINE_MAX - 1;
    }
    memcpy(s_line, new_line, new_len);
    s_line[new_len] = '\0';
    s_line_pos = new_len;
    printf("%.*s", new_len, s_line);
}

static void history_push(const char *cmd)
{
    strncpy(s_history[s_history_write], cmd, INPUT_LINE_MAX - 1);
    s_history[s_history_write][INPUT_LINE_MAX - 1] = '\0';
    s_history_write = (s_history_write + 1) % HISTORY_SIZE;
    if (s_history_count < HISTORY_SIZE)
    {
        s_history_count++;
    }
}

static void history_prev(void)
{
    if (s_history_count == 0)
    {
        return;
    }

    if (s_history_browse < 0)
    {
        memcpy(s_saved_line, s_line, INPUT_LINE_MAX);
        s_saved_pos = s_line_pos;
        s_history_browse = 0;
    }
    else if (s_history_browse < s_history_count - 1)
    {
        s_history_browse++;
    }
    else
    {
        return;
    }

    int idx = (s_history_write - 1 - s_history_browse + HISTORY_SIZE) % HISTORY_SIZE;
    replace_line(s_history[idx]);
}

static void history_next(void)
{
    if (s_history_browse < 0)
    {
        return;
    }

    s_history_browse--;

    if (s_history_browse < 0)
    {
        s_saved_line[s_saved_pos] = '\0';
        replace_line(s_saved_line);
        return;
    }

    int idx = (s_history_write - 1 - s_history_browse + HISTORY_SIZE) % HISTORY_SIZE;
    replace_line(s_history[idx]);
}

/* ── Line editor ───────────────────────────────────────────── */

static void print_prompt(void)
{
    const char *prompt = shell_get_prompt();
    if (prompt != NULL)
    {
        printf("%s", prompt);
    }
}

static void execute_line(void)
{
    s_line[s_line_pos] = '\0';
    printf("\n");

    if (s_line_pos > 0)
    {
        history_push(s_line);

        int ret = 0;
        esp_err_t err = esp_console_run(s_line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unknown command: %s\n", s_line);
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            /* Empty or whitespace-only */
        }
        else if (err != ESP_OK)
        {
            printf("Error: %s\n", esp_err_to_name(err));
        }
    }

    s_line_pos = 0;
    s_history_browse = -1;
    print_prompt();
}

static void handle_normal_char(uint8_t ch)
{
    if (ch == '\n' || ch == '\r')
    {
        execute_line();
        return;
    }

    if (ch == '\b' || ch == 0x7F)
    {
        if (s_line_pos > 0)
        {
            s_line_pos--;
            printf("\b \b");
        }
        return;
    }

    if (ch == 0x03) /* Ctrl+C */
    {
        printf("^C\n");
        s_line_pos = 0;
        s_history_browse = -1;
        print_prompt();
        return;
    }

    if (ch == 0x1B)
    {
        s_parse_state = ST_ESC;
        return;
    }

    /* Ignore non-printable control characters */
    if (ch < 0x20 && ch != '\t')
    {
        return;
    }

    if (s_line_pos < INPUT_LINE_MAX - 1)
    {
        s_line[s_line_pos++] = (char)ch;
        printf("%c", ch);
    }
}

static void process_byte(uint8_t ch)
{
    switch (s_parse_state)
    {
    case ST_NORMAL:
        handle_normal_char(ch);
        break;

    case ST_ESC:
        if (ch == '[')
        {
            s_parse_state = ST_CSI;
            return;
        }
        s_parse_state = ST_NORMAL;
        break;

    case ST_CSI:
        s_parse_state = ST_NORMAL;
        if (ch == 'A')
        {
            history_prev();
        }
        else if (ch == 'B')
        {
            history_next();
        }
        break;
    }
}

/* ── Tasks ─────────────────────────────────────────────────── */

static void uart_rx_task(void *arg)
{
    (void)arg;
    uint8_t buf[32];
    for (;;)
    {
        int n = uart_read_bytes(UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (n > 0)
        {
            xStreamBufferSend(s_input_stream, buf, (size_t)n, portMAX_DELAY);
        }
    }
}

static void input_task(void *arg)
{
    (void)arg;
    print_prompt();
    fflush(stdout);

    uint8_t buf[32];
    for (;;)
    {
        size_t n = xStreamBufferReceive(s_input_stream, buf, sizeof(buf), portMAX_DELAY);
        for (size_t i = 0; i < n; i++)
        {
            process_byte(buf[i]);
        }
        fflush(stdout);
    }
}

/* ── Public API ────────────────────────────────────────────── */

void shell_feed_input(const char *data, size_t len)
{
    if (s_input_stream == NULL || data == NULL || len == 0)
    {
        return;
    }
    xStreamBufferSend(s_input_stream, data, len, pdMS_TO_TICKS(50));
}

esp_err_t shell_input_init(void)
{
    esp_err_t err = uart_driver_install(UART_NUM, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }
    uart_vfs_dev_use_driver(UART_NUM);

    s_input_stream = xStreamBufferCreate(INPUT_STREAM_SIZE, 1);
    if (s_input_stream == NULL)
    {
        ESP_LOGE(TAG, "Failed to create input stream buffer");
        return ESP_ERR_NO_MEM;
    }

    s_history_browse = -1;
    s_parse_state = ST_NORMAL;

    BaseType_t rc;

    rc = xTaskCreate(uart_rx_task, "uart_rx", RX_TASK_STACK, NULL, RX_TASK_PRIO, NULL);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create UART RX task");
        return ESP_ERR_NO_MEM;
    }

    rc = xTaskCreate(input_task, "shell_in", INPUT_TASK_STACK, NULL, INPUT_TASK_PRIO, NULL);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create input task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Shell input ready");
    return ESP_OK;
}
