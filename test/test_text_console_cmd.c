#include "unity.h"

#include "mock_console.h"
#include "mock_text_console.h"
#include "text_console.h"

void setUp(void)
{
    mock_console_reset();
    mock_text_console_reset();
    text_console_register_commands();
}

void tearDown(void) {}

static void test_clear_command_clears_console(void)
{
    char *argv[] = {"clear"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("clear", 1, argv));
    TEST_ASSERT_EQUAL(1, mock_text_console_clear_count());
}

static void test_clear_command_rejects_arguments(void)
{
    char *argv[] = {"clear", "extra"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("clear", 2, argv));
    TEST_ASSERT_EQUAL(0, mock_text_console_clear_count());
}

static void test_logging_command_sets_debug_level(void)
{
    char *argv[] = {"logging", "debug"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("logging", 2, argv));
    TEST_ASSERT_EQUAL(TEXT_CONSOLE_LOG_DEBUG, text_console_get_log_level());
    TEST_ASSERT_EQUAL_STRING("debug", text_console_log_level_name(text_console_get_log_level()));
}

static void test_logging_command_rejects_invalid_level(void)
{
    char *set_warn[] = {"logging", "warn"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("logging", 2, set_warn));

    char *invalid[] = {"logging", "verbose"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("logging", 2, invalid));
    TEST_ASSERT_EQUAL(TEXT_CONSOLE_LOG_WARN, text_console_get_log_level());
}

static void test_logging_command_reports_current_level(void)
{
    char *set_debug[] = {"logging", "debug"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("logging", 2, set_debug));

    char *query[] = {"logging"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("logging", 1, query));
    TEST_ASSERT_EQUAL(TEXT_CONSOLE_LOG_DEBUG, text_console_get_log_level());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_command_clears_console);
    RUN_TEST(test_clear_command_rejects_arguments);
    RUN_TEST(test_logging_command_sets_debug_level);
    RUN_TEST(test_logging_command_rejects_invalid_level);
    RUN_TEST(test_logging_command_reports_current_level);
    return UNITY_END();
}
