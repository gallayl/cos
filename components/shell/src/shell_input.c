#include "shell_input.h"
#include "shell.h"
#include "shell_alias.h"
#include "shell_completion.h"
#include "shell_script.h"

#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "shell_input";

#define INPUT_LINE_MAX 128
#define INPUT_STREAM_SIZE 256
#define INPUT_CANCEL_REQUEST 0xFF
#define PROMPT_EOL_GRACE_MS 50
#define HISTORY_SIZE 8
#define UART_RX_BUF_SIZE 256
#define UART_NUM CONFIG_ESP_CONSOLE_UART_NUM

#define RX_TASK_STACK 2048
#define RX_TASK_PRIO 5
#define INPUT_TASK_STACK 16384
#define INPUT_TASK_PRIO 4

static StreamBufferHandle_t s_input_stream;
static TaskHandle_t s_input_task;
static atomic_bool s_line_input_owned;
static _Atomic(void *) s_line_input_owner;

/* ── Line buffer ───────────────────────────────────────────── */

static char s_line[INPUT_LINE_MAX];
static int s_line_pos;
static bool s_ignore_lf;
static bool s_ignore_prompt_eol;

/* ── Command history ───────────────────────────────────────── */

static char s_history[HISTORY_SIZE][INPUT_LINE_MAX];
static int s_history_count;
static int s_history_write;  /* next slot to write */
static int s_history_browse; /* browse offset, -1 = live line */
static char s_saved_line[INPUT_LINE_MAX];
static int s_saved_pos;

/* ── ESC sequence parser state ─────────────────────────────── */

typedef enum
{
    ST_NORMAL,
    ST_ESC,
    ST_CSI,
    ST_SS3,
} input_state_t;

static input_state_t s_parse_state;

typedef struct
{
    bool active;
    char buffer[INPUT_LINE_MAX];
    size_t length;
    shell_line_input_callback_t callback;
    void *context;
} line_input_request_t;

static line_input_request_t s_line_input;
static input_state_t s_line_escape_state;

typedef struct
{
    bool active;
    bool character_only;
    shell_line_input_callback_t callback;
    void *context;
    input_state_t escape_state;
    int csi_param;
    bool csi_param_any;
    bool csi_skip_param;
} key_input_request_t;

static key_input_request_t s_key_input;

#define KEY_SEQUENCE_TIMEOUT_MS 50

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

static void finish_line_input(esp_err_t result)
{
    shell_line_input_callback_t callback = s_line_input.callback;
    void *context = s_line_input.context;
    char line[INPUT_LINE_MAX];
    if (result == ESP_OK)
    {
        memcpy(line, s_line_input.buffer, s_line_input.length);
        line[s_line_input.length] = '\0';
    }

    s_line_input.active = false;
    atomic_store(&s_line_input_owned, false);
    atomic_store(&s_line_input_owner, NULL);
    s_line_input.length = 0;
    s_line_input.callback = NULL;
    s_line_input.context = NULL;
    s_line_escape_state = ST_NORMAL;

    if (result == ESP_OK)
    {
        printf("\n");
        callback(result, line, context);
    }
    else
    {
        printf("^C\n");
        callback(result, NULL, context);
    }

    if (!s_line_input.active)
    {
        print_prompt();
    }
}

static void process_line_input_byte(uint8_t ch)
{
    if (s_line_escape_state == ST_ESC)
    {
        s_line_escape_state = (ch == '[' || ch == 'O') ? ST_CSI : ST_NORMAL;
        return;
    }
    if (s_line_escape_state == ST_CSI)
    {
        if (ch >= 0x40 && ch <= 0x7E)
        {
            s_line_escape_state = ST_NORMAL;
        }
        return;
    }
    if (ch == 0x1B)
    {
        s_line_escape_state = ST_ESC;
        return;
    }
    if (ch == '\n' || ch == '\r')
    {
        finish_line_input(ESP_OK);
        return;
    }
    if (ch == 0x03)
    {
        finish_line_input(ESP_ERR_INVALID_STATE);
        return;
    }
    if (ch == '\b' || ch == 0x7F)
    {
        if (s_line_input.length > 0)
        {
            s_line_input.length--;
            s_line_input.buffer[s_line_input.length] = '\0';
            printf("\b \b");
        }
        return;
    }
    if (ch >= 0x20 && ch < 0x7F && s_line_input.length < INPUT_LINE_MAX - 1)
    {
        s_line_input.buffer[s_line_input.length++] = (char)ch;
        s_line_input.buffer[s_line_input.length] = '\0';
        printf("%c", ch);
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
        char expanded_line[INPUT_LINE_MAX];
        esp_err_t err = shell_alias_expand(s_line, expanded_line, sizeof(expanded_line));
        if (err == ESP_OK)
        {
            err = shell_dispatch_expanded_line(expanded_line, &ret);
        }
        else if (err == ESP_ERR_INVALID_SIZE)
        {
            printf("Alias expansion exceeds the command-line limit\n");
            err = ESP_OK;
        }

        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unknown command: %s\n", s_line);
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            /* Empty or whitespace-only, or alias expansion issue already handled */
        }
        else if (err != ESP_OK)
        {
            /* Dispatch infrastructure errors (for example NO_MEM). */
        }
        (void)ret;
    }

    s_line_pos = 0;
    s_history_browse = -1;
    if (!s_line_input.active)
    {
        print_prompt();
    }
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

    if (ch == '\t')
    {
        s_line[s_line_pos] = '\0';
        bool changed = false;
        esp_err_t err = shell_complete_line(s_line, sizeof(s_line), &changed);
        if (err != ESP_OK)
        {
            printf("\nCompletion failed: %s\n", esp_err_to_name(err));
            print_prompt();
            printf("%s", s_line);
        }
        else if (changed)
        {
            erase_line_on_screen();
            s_line_pos = (int)strlen(s_line);
            printf("%s", s_line);
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
    if (ch < 0x20)
    {
        return;
    }

    if (s_line_pos < INPUT_LINE_MAX - 1)
    {
        s_line[s_line_pos++] = (char)ch;
        printf("%c", ch);
    }
}

static void finish_key_input(esp_err_t result, const char *key)
{
    shell_line_input_callback_t callback = s_key_input.callback;
    void *context = s_key_input.context;
    char stored[16];
    const char *delivered = NULL;

    if (result == ESP_OK && key != NULL)
    {
        size_t index = 0;
        while (key[index] != '\0' && index + 1 < sizeof(stored))
        {
            stored[index] = key[index];
            index++;
        }
        stored[index] = '\0';
        delivered = stored;
    }

    memset(&s_key_input, 0, sizeof(s_key_input));
    atomic_store(&s_line_input_owned, s_line_input.active);
    atomic_store(&s_line_input_owner, s_line_input.active ? s_line_input.context : NULL);

    if (callback != NULL)
    {
        callback(result, delivered, context);
    }
}

static void deliver_key_name(const char *name)
{
    if (!s_key_input.active || name == NULL || name[0] == '\0')
    {
        return;
    }
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (s_key_input.character_only &&
        (name[1] != '\0' ||
         (unsigned char)name[0] <
             0x20)) // NOLINT(readability-implicit-bool-conversion) - bool/comparison mix in C condition
    {
        return;
    }
    finish_key_input(ESP_OK, name);
}

static const char *csi_key_name(int param, char final_byte)
{
    switch (final_byte)
    {
    case 'A':
        return "up";
    case 'B':
        return "down";
    case 'C':
        return "right";
    case 'D':
        return "left";
    case 'H':
        return "home";
    case 'F':
        return "end";
    case '~':
        switch (param)
        {
        case 1:
        case 7:
            return "home";
        case 2:
            return "insert";
        case 3:
            return "delete";
        case 4:
        case 8:
            return "end";
        case 5:
            return "pageup";
        case 6:
            return "pagedown";
        case 11:
            return "f1";
        case 12:
            return "f2";
        case 13:
            return "f3";
        case 14:
            return "f4";
        case 15:
            return "f5";
        case 17:
            return "f6";
        case 18:
            return "f7";
        case 19:
            return "f8";
        case 20:
            return "f9";
        case 21:
            return "f10";
        case 23:
            return "f11";
        case 24:
            return "f12";
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}

static const char *ss3_key_name(char final_byte)
{
    switch (final_byte)
    {
    case 'A':
        return "up";
    case 'B':
        return "down";
    case 'C':
        return "right";
    case 'D':
        return "left";
    case 'H':
        return "home";
    case 'F':
        return "end";
    case 'P':
        return "f1";
    case 'Q':
        return "f2";
    case 'R':
        return "f3";
    case 'S':
        return "f4";
    default:
        return NULL;
    }
}

static void reset_key_sequence(void)
{
    s_key_input.escape_state = ST_NORMAL;
    s_key_input.csi_param = 0;
    s_key_input.csi_param_any = false;
    s_key_input.csi_skip_param = false;
}

static void expire_pending_key_sequence(void)
{
    if (!s_key_input.active || s_key_input.escape_state == ST_NORMAL)
    {
        return;
    }

    reset_key_sequence();
    if (!s_key_input.character_only)
    {
        finish_key_input(ESP_OK, "escape");
    }
}

static bool key_sequence_is_pending(void)
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    return s_key_input.active &&
           (s_key_input.escape_state !=
            ST_NORMAL); // NOLINT(readability-implicit-bool-conversion) - bool expression in C return
}

static void process_byte(uint8_t ch);
// NOLINTNEXTLINE(misc-no-recursion) - mutually recursive with process_byte/reprocess_after_escape for ESC reprocessing;
// bounded by input bytes.
static void process_key_byte(uint8_t ch);

// NOLINTNEXTLINE(misc-no-recursion) - small, bounded recursion when reprocessing a byte after an incomplete escape
// sequence.
// NOLINTNEXTLINE(misc-no-recursion) - bounded/intentional; preserve behavior
static void reprocess_after_escape(uint8_t ch)
{
    if (s_key_input.active)
    {
        process_key_byte(ch);
        return;
    }
    process_byte(ch);
}

// NOLINTNEXTLINE(misc-no-recursion) - mutually recursive with process_byte for key/escape handling; bounded by input
// bytes.
// NOLINTNEXTLINE(misc-no-recursion) - bounded/intentional; preserve behavior
static void process_key_byte(uint8_t ch)
{
    if (!s_key_input.active)
    {
        return;
    }
    if (ch == 0x03)
    {
        finish_key_input(ESP_ERR_INVALID_STATE, NULL);
        return;
    }

    if (s_key_input.escape_state == ST_ESC)
    {
        if (ch == '[')
        {
            s_key_input.escape_state = ST_CSI;
            s_key_input.csi_param = 0;
            s_key_input.csi_param_any = false;
            s_key_input.csi_skip_param = false;
            return;
        }
        if (ch == 'O')
        {
            s_key_input.escape_state = ST_SS3;
            return;
        }

        reset_key_sequence();
        if (!s_key_input.character_only)
        {
            finish_key_input(ESP_OK, "escape");
        }
        reprocess_after_escape(ch);
        return;
    }

    if (s_key_input.escape_state == ST_SS3)
    {
        reset_key_sequence();
        deliver_key_name(ss3_key_name((char)ch));
        return;
    }

    if (s_key_input.escape_state == ST_CSI)
    {
        if (ch >= '0' && ch <= '9')
        {
            if (!s_key_input.csi_skip_param && s_key_input.csi_param < 1000)
            {
                s_key_input.csi_param = s_key_input.csi_param * 10 + (ch - '0');
                s_key_input.csi_param_any = true;
            }
            return;
        }
        if (ch == ';')
        {
            s_key_input.csi_skip_param = true;
            return;
        }
        if (ch >= 0x40 && ch <= 0x7E)
        {
            // NOLINTNEXTLINE(readability-implicit-bool-conversion)
            const int param = s_key_input.csi_param_any
                                  ? s_key_input.csi_param
                                  : 0; // NOLINT(readability-implicit-bool-conversion) - bool condition in C ternary
            reset_key_sequence();
            deliver_key_name(csi_key_name(param, (char)ch));
            return;
        }
        reset_key_sequence();
        return;
    }

    if (ch == 0x1B)
    {
        s_key_input.escape_state = ST_ESC;
        return;
    }
    if (ch == '\n' || ch == '\r')
    {
        deliver_key_name("enter");
        return;
    }
    if (ch == '\b' || ch == 0x7F)
    {
        deliver_key_name("backspace");
        return;
    }
    if (ch == '\t')
    {
        deliver_key_name("tab");
        return;
    }
    if (ch >= 0x20 && ch < 0x7F)
    {
        const char text[2] = {(char)ch, '\0'};
        deliver_key_name(text);
    }
}

// NOLINTNEXTLINE(misc-no-recursion) - mutually recursive with process_key_byte for key/escape handling; bounded by
// input bytes.
// NOLINTNEXTLINE(misc-no-recursion) - bounded/intentional; preserve behavior
static void process_byte(uint8_t ch)
{
    if (s_ignore_lf)
    {
        s_ignore_lf = false;
        if (ch == '\n')
        {
            s_ignore_prompt_eol = false;
            return;
        }
    }
    if (s_ignore_prompt_eol)
    {
        s_ignore_prompt_eol = false;
        if (ch == '\r' || ch == '\n')
        {
            return;
        }
    }
    if (ch == '\r')
    {
        s_ignore_lf = true;
    }

    if (ch == INPUT_CANCEL_REQUEST)
    {
        if (s_key_input.active)
        {
            finish_key_input(ESP_ERR_INVALID_STATE, NULL);
        }
        else if (s_line_input.active)
        {
            finish_line_input(ESP_ERR_INVALID_STATE);
        }
        return;
    }

    if (s_key_input.active)
    {
        process_key_byte(ch);
        return;
    }

    if (s_line_input.active)
    {
        process_line_input_byte(ch);
        return;
    }

    switch (s_parse_state)
    {
    case ST_NORMAL:
        handle_normal_char(ch);
        if ((ch == '\r' || ch == '\n') &&
            s_line_input.active) // NOLINT(readability-implicit-bool-conversion) - bool/comparison mix in C condition
        {
            s_ignore_prompt_eol = true;
        }
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

    case ST_SS3:
        s_parse_state = ST_NORMAL;
        break;
    }
}

static void process_input_buffer(const uint8_t *buf, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        process_byte(buf[i]);
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
        const TickType_t wait =
            key_sequence_is_pending() // NOLINT(readability-implicit-bool-conversion) - bool condition in C ternary
                ? pdMS_TO_TICKS(KEY_SEQUENCE_TIMEOUT_MS)
                : portMAX_DELAY;
        size_t n = xStreamBufferReceive(s_input_stream, buf, sizeof(buf), wait);
        if (n == 0)
        {
            expire_pending_key_sequence();
            continue;
        }
        process_input_buffer(buf, n);
        if (s_ignore_prompt_eol)
        {
            n = xStreamBufferReceive(s_input_stream, buf, sizeof(buf), pdMS_TO_TICKS(PROMPT_EOL_GRACE_MS));
            if (n > 0)
            {
                process_input_buffer(buf, n);
            }
            s_ignore_prompt_eol = false;
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

esp_err_t shell_input_read_line(const char *prompt, shell_line_input_callback_t callback, void *context)
{
    if (prompt == NULL || callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_input_stream == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_input_task != NULL && xTaskGetCurrentTaskHandle() != s_input_task)
    {
        return ESP_ERR_INVALID_STATE;
    }
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (s_line_input.active ||
        s_key_input.active) // NOLINT(readability-implicit-bool-conversion) - bools in C logical op
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_line_input, 0, sizeof(s_line_input));
    s_line_input.active = true;
    s_line_input.callback = callback;
    s_line_input.context = context;
    atomic_store(&s_line_input_owned, true);
    atomic_store(&s_line_input_owner, context);
    s_line_escape_state = ST_NORMAL;
    printf("%s", prompt);
    fflush(stdout);
    return ESP_OK;
}

esp_err_t shell_input_cancel_line(void *context)
{
    if (!atomic_load(&s_line_input_owned))
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (context != NULL && context != atomic_load(&s_line_input_owner))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t cancel = INPUT_CANCEL_REQUEST;
    if (xStreamBufferSend(s_input_stream, &cancel, sizeof(cancel), pdMS_TO_TICKS(50)) != sizeof(cancel))
    {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

bool shell_input_is_reading_line(void)
{
    return atomic_load(&s_line_input_owned);
}

static esp_err_t begin_key_input(bool character_only, shell_line_input_callback_t callback, void *context)
{
    if (callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_input_stream == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_input_task != NULL && xTaskGetCurrentTaskHandle() != s_input_task)
    {
        return ESP_ERR_INVALID_STATE;
    }
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (s_line_input.active ||
        s_key_input.active) // NOLINT(readability-implicit-bool-conversion) - bools in C logical op
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_key_input, 0, sizeof(s_key_input));
    s_key_input.active = true;
    s_key_input.character_only = character_only;
    s_key_input.callback = callback;
    s_key_input.context = context;
    s_key_input.escape_state = ST_NORMAL;
    atomic_store(&s_line_input_owned, true);
    atomic_store(&s_line_input_owner, context);
    return ESP_OK;
}

esp_err_t shell_input_read_key(shell_line_input_callback_t callback, void *context)
{
    return begin_key_input(false, callback, context);
}

esp_err_t shell_input_read_char(shell_line_input_callback_t callback, void *context)
{
    return begin_key_input(true, callback, context);
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
    s_ignore_lf = false;
    s_ignore_prompt_eol = false;
    atomic_init(&s_line_input_owned, false);
    atomic_init(&s_line_input_owner, NULL);

    static TaskHandle_t s_uart_rx_task;
    BaseType_t rc;

    rc = xTaskCreate(uart_rx_task, "uart_rx", RX_TASK_STACK, NULL, RX_TASK_PRIO, &s_uart_rx_task);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create UART RX task");
        return ESP_ERR_NO_MEM;
    }

    rc = xTaskCreate(input_task, "shell_in", INPUT_TASK_STACK, NULL, INPUT_TASK_PRIO, &s_input_task);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create input task");
        vTaskDelete(s_uart_rx_task);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Shell input ready");
    return ESP_OK;
}
