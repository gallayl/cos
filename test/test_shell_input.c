#include "unity.h"

#include <stdio.h>
#include <string.h>

/* Pull in the implementation to test static functions directly */
#include "shell_input.c"

/* Mock shell_get_prompt (defined in shell.c, not linked here) */
static const char *s_mock_prompt = "COS> ";

const char *shell_get_prompt(void)
{
    return s_mock_prompt;
}

static int s_completion_calls;
static int s_line_callback_calls;
static esp_err_t s_line_callback_result;
static char s_line_callback_value[INPUT_LINE_MAX];
static int s_test_stream;
static int s_other_task;

static void line_input_callback(esp_err_t result, const char *line, void *context)
{
    TEST_ASSERT_EQUAL_PTR(&s_test_stream, context);
    s_line_callback_calls++;
    s_line_callback_result = result;
    if (line != NULL)
    {
        strncpy(s_line_callback_value, line, sizeof(s_line_callback_value) - 1);
        s_line_callback_value[sizeof(s_line_callback_value) - 1] = '\0';
    }
}

esp_err_t shell_complete_line(char *line, size_t capacity, bool *changed)
{
    s_completion_calls++;
    if (capacity < sizeof("help "))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    strcpy(line, "help ");
    *changed = true;
    return ESP_OK;
}

static void reset_state(void)
{
    s_line_pos = 0;
    memset(s_line, 0, sizeof(s_line));
    s_parse_state = ST_NORMAL;
    s_history_count = 0;
    s_history_write = 0;
    s_history_browse = -1;
    memset(s_history, 0, sizeof(s_history));
    memset(s_saved_line, 0, sizeof(s_saved_line));
    s_saved_pos = 0;
    s_completion_calls = 0;
    s_line_callback_calls = 0;
    s_line_callback_result = ESP_OK;
    memset(s_line_callback_value, 0, sizeof(s_line_callback_value));
    memset(&s_line_input, 0, sizeof(s_line_input));
    s_line_escape_state = ST_NORMAL;
    atomic_store(&s_line_input_owned, false);
    atomic_store(&s_line_input_owner, NULL);
    s_input_task = NULL;
    s_input_stream = &s_test_stream;
}

void setUp(void)
{
    reset_state();
}

void tearDown(void) {}

/* ── Helper: feed a string through process_byte ──────────── */

static void feed(const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        process_byte((uint8_t)str[i]);
    }
}

/* ── Basic character input ───────────────────────────────── */

static void test_typing_builds_line(void)
{
    feed("hello");
    TEST_ASSERT_EQUAL(5, s_line_pos);
    TEST_ASSERT_EQUAL_STRING_LEN("hello", s_line, 5);
}

static void test_line_null_terminated_on_enter(void)
{
    feed("abc");
    process_byte('\n');
    /* After enter, line is reset */
    TEST_ASSERT_EQUAL(0, s_line_pos);
}

/* ── Backspace ───────────────────────────────────────────── */

static void test_backspace_removes_char(void)
{
    feed("abc");
    process_byte('\b');
    TEST_ASSERT_EQUAL(2, s_line_pos);
    TEST_ASSERT_EQUAL_STRING_LEN("ab", s_line, 2);
}

static void test_backspace_at_empty_does_nothing(void)
{
    process_byte('\b');
    TEST_ASSERT_EQUAL(0, s_line_pos);
}

static void test_delete_key_also_backspaces(void)
{
    feed("xy");
    process_byte(0x7F);
    TEST_ASSERT_EQUAL(1, s_line_pos);
}

/* ── Ctrl+C ──────────────────────────────────────────────── */

static void test_ctrl_c_clears_line(void)
{
    feed("some text");
    process_byte(0x03);
    TEST_ASSERT_EQUAL(0, s_line_pos);
}

/* ── Line buffer overflow protection ─────────────────────── */

static void test_line_max_length(void)
{
    for (int i = 0; i < INPUT_LINE_MAX + 10; i++)
    {
        process_byte('x');
    }
    TEST_ASSERT_EQUAL(INPUT_LINE_MAX - 1, s_line_pos);
}

/* ── Non-printable control chars ignored ─────────────────── */

static void test_control_chars_ignored(void)
{
    feed("a");
    process_byte(0x01); /* SOH */
    process_byte(0x02); /* STX */
    process_byte(0x04); /* EOT */
    feed("b");
    TEST_ASSERT_EQUAL(2, s_line_pos);
    TEST_ASSERT_EQUAL_STRING_LEN("ab", s_line, 2);
}

static void test_tab_completes_without_becoming_line_input(void)
{
    feed("he");
    process_byte('\t');
    TEST_ASSERT_EQUAL(1, s_completion_calls);
    TEST_ASSERT_EQUAL_STRING("help ", s_line);
    TEST_ASSERT_EQUAL(5, s_line_pos);
}

/* ── ESC sequence parsing ────────────────────────────────── */

static void test_esc_bracket_then_unknown_resets_state(void)
{
    feed("a");
    process_byte(0x1B); /* ESC */
    TEST_ASSERT_EQUAL(ST_ESC, s_parse_state);
    process_byte('[');
    TEST_ASSERT_EQUAL(ST_CSI, s_parse_state);
    process_byte('Z'); /* Unknown CSI */
    TEST_ASSERT_EQUAL(ST_NORMAL, s_parse_state);
    TEST_ASSERT_EQUAL(1, s_line_pos);
}

static void test_esc_without_bracket_resets(void)
{
    process_byte(0x1B);
    process_byte('O');
    TEST_ASSERT_EQUAL(ST_NORMAL, s_parse_state);
}

/* ── History ─────────────────────────────────────────────── */

static void test_history_push_and_prev(void)
{
    feed("cmd1");
    process_byte('\n');
    feed("cmd2");
    process_byte('\n');

    /* Arrow up (ESC [ A) should recall "cmd2" */
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');

    TEST_ASSERT_EQUAL_STRING_LEN("cmd2", s_line, 4);
    TEST_ASSERT_EQUAL(4, s_line_pos);
}

static void test_history_prev_then_next(void)
{
    feed("first");
    process_byte('\n');
    feed("second");
    process_byte('\n');

    /* Up twice */
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');
    TEST_ASSERT_EQUAL_STRING_LEN("first", s_line, 5);

    /* Down once -> back to "second" */
    process_byte(0x1B);
    process_byte('[');
    process_byte('B');
    TEST_ASSERT_EQUAL_STRING_LEN("second", s_line, 6);
}

static void test_history_down_past_end_restores_live(void)
{
    feed("old");
    process_byte('\n');
    feed("typing");

    /* Up */
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');
    TEST_ASSERT_EQUAL_STRING_LEN("old", s_line, 3);

    /* Down -> restore "typing" */
    process_byte(0x1B);
    process_byte('[');
    process_byte('B');
    TEST_ASSERT_EQUAL_STRING_LEN("typing", s_line, 6);
}

static void test_history_empty_up_does_nothing(void)
{
    feed("live");
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');
    TEST_ASSERT_EQUAL_STRING_LEN("live", s_line, 4);
}

static void test_history_down_without_browsing_does_nothing(void)
{
    feed("live");
    process_byte(0x1B);
    process_byte('[');
    process_byte('B');
    TEST_ASSERT_EQUAL_STRING_LEN("live", s_line, 4);
}

static void test_async_line_input_collects_text_and_completes(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_input_read_line("Name: ", line_input_callback, &s_test_stream));
    TEST_ASSERT_TRUE(shell_input_is_reading_line());
    feed("Berry");
    process_byte('\n');

    TEST_ASSERT_FALSE(shell_input_is_reading_line());
    TEST_ASSERT_EQUAL(1, s_line_callback_calls);
    TEST_ASSERT_EQUAL(ESP_OK, s_line_callback_result);
    TEST_ASSERT_EQUAL_STRING("Berry", s_line_callback_value);
}

static void test_async_line_input_backspace_and_cancel(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_input_read_line("Value: ", line_input_callback, &s_test_stream));
    feed("abcd");
    process_byte('\b');
    feed("e");
    process_byte(0x03);

    TEST_ASSERT_FALSE(shell_input_is_reading_line());
    TEST_ASSERT_EQUAL(1, s_line_callback_calls);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, s_line_callback_result);
    TEST_ASSERT_EQUAL_STRING("", s_line_callback_value);
}

static void test_async_line_input_rejects_concurrent_requests(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_input_read_line("First: ", line_input_callback, &s_test_stream));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, shell_input_read_line("Second: ", line_input_callback, NULL));
    TEST_ASSERT_EQUAL(ESP_OK, shell_input_cancel_line(&s_test_stream));
    TEST_ASSERT_TRUE(shell_input_is_reading_line());
    process_byte(INPUT_CANCEL_REQUEST);
    TEST_ASSERT_EQUAL(1, s_line_callback_calls);
    TEST_ASSERT_FALSE(shell_input_is_reading_line());
}

static void test_async_line_input_rejects_other_task_requests(void)
{
    s_input_task = &s_other_task;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, shell_input_read_line("Name: ", line_input_callback, &s_test_stream));
    TEST_ASSERT_FALSE(shell_input_is_reading_line());
}

static void test_async_line_input_cross_task_cancel_is_queued(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_input_read_line("Name: ", line_input_callback, &s_test_stream));
    s_input_task = &s_other_task;

    TEST_ASSERT_EQUAL(ESP_OK, shell_input_cancel_line(&s_test_stream));
    TEST_ASSERT_TRUE(shell_input_is_reading_line());

    process_byte(INPUT_CANCEL_REQUEST);
    TEST_ASSERT_FALSE(shell_input_is_reading_line());
    TEST_ASSERT_EQUAL(1, s_line_callback_calls);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, s_line_callback_result);
}

static void test_history_wraps_around(void)
{
    for (int i = 0; i < HISTORY_SIZE + 2; i++)
    {
        char cmd[16];
        int len = snprintf(cmd, sizeof(cmd), "cmd%d", i);
        for (int j = 0; j < len; j++)
        {
            process_byte((uint8_t)cmd[j]);
        }
        process_byte('\n');
    }

    /* Up should recall the last one (cmd<HISTORY_SIZE+1>) */
    process_byte(0x1B);
    process_byte('[');
    process_byte('A');

    char expected[16];
    snprintf(expected, sizeof(expected), "cmd%d", HISTORY_SIZE + 1);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, s_line, strlen(expected));
}

/* ── main ────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* Basic input */
    RUN_TEST(test_typing_builds_line);
    RUN_TEST(test_line_null_terminated_on_enter);

    /* Backspace */
    RUN_TEST(test_backspace_removes_char);
    RUN_TEST(test_backspace_at_empty_does_nothing);
    RUN_TEST(test_delete_key_also_backspaces);

    /* Ctrl+C */
    RUN_TEST(test_ctrl_c_clears_line);

    /* Overflow */
    RUN_TEST(test_line_max_length);

    /* Control characters */
    RUN_TEST(test_control_chars_ignored);
    RUN_TEST(test_tab_completes_without_becoming_line_input);

    /* ESC sequences */
    RUN_TEST(test_esc_bracket_then_unknown_resets_state);
    RUN_TEST(test_esc_without_bracket_resets);

    /* History */
    RUN_TEST(test_history_push_and_prev);
    RUN_TEST(test_history_prev_then_next);
    RUN_TEST(test_history_down_past_end_restores_live);
    RUN_TEST(test_history_empty_up_does_nothing);
    RUN_TEST(test_history_down_without_browsing_does_nothing);
    RUN_TEST(test_async_line_input_collects_text_and_completes);
    RUN_TEST(test_async_line_input_backspace_and_cancel);
    RUN_TEST(test_async_line_input_rejects_concurrent_requests);
    RUN_TEST(test_async_line_input_rejects_other_task_requests);
    RUN_TEST(test_async_line_input_cross_task_cancel_is_queued);
    RUN_TEST(test_history_wraps_around);

    return UNITY_END();
}
