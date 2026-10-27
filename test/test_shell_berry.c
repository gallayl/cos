#include "unity.h"

#include "mock_berry.h"
#include "mock_console.h"
#include "shell_cmds.h"

void setUp(void)
{
    mock_console_reset();
    mock_berry_reset();
    shell_register_berry_commands();
}

void tearDown(void) {}

static void test_run_resolves_relative_path_from_shell_directory(void)
{
    char *argv[] = {"berry", "run", "apps/demo.be"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("/flash/apps/demo.be", mock_berry_last_path());
}

static void test_run_preserves_absolute_sdcard_path(void)
{
    char *argv[] = {"berry", "run", "/sdcard/berry/demo.be"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("/sdcard/berry/demo.be", mock_berry_last_path());
}

static void test_eval_joins_source_arguments(void)
{
    char *argv[] = {"berry", "eval", "print(1", "+", "2)"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 5, argv));
    TEST_ASSERT_EQUAL_STRING("print(1 + 2)", mock_berry_last_source());
    TEST_ASSERT_EQUAL_STRING("shell", mock_berry_last_source_name());
}

static void test_init_error_fails_command(void)
{
    mock_berry_set_init_result(ESP_ERR_NO_MEM);
    char *argv[] = {"berry", "eval", "print(1)"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_source());
}

static void test_execution_error_fails_command(void)
{
    mock_berry_set_run_result(ESP_FAIL);
    char *argv[] = {"berry", "run", "demo.be"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("/flash/demo.be", mock_berry_last_path());
}

static void test_unknown_operation_fails_command(void)
{
    char *argv[] = {"berry", "open", "demo.be"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
}

static void test_missing_arguments_fails_command(void)
{
    char *argv[] = {"berry", "run"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 2, argv));
}

static void test_close_stops_active_app(void)
{
    char *argv[] = {"berry", "close"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));

    mock_berry_set_close_result(ESP_FAIL);
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 2, argv));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_run_resolves_relative_path_from_shell_directory);
    RUN_TEST(test_run_preserves_absolute_sdcard_path);
    RUN_TEST(test_eval_joins_source_arguments);
    RUN_TEST(test_init_error_fails_command);
    RUN_TEST(test_execution_error_fails_command);
    RUN_TEST(test_unknown_operation_fails_command);
    RUN_TEST(test_missing_arguments_fails_command);
    RUN_TEST(test_close_stops_active_app);
    return UNITY_END();
}
