#include "unity.h"

#include "mock_berry.h"
#include "mock_console.h"
#include "mock_filesystem.h"
#include "mock_network.h"
#include "mock_system.h"
#include "shell.h"
#include "shell_cmds.h"

void setUp(void)
{
    mock_console_reset();
    mock_filesystem_reset();
    mock_berry_reset();
    mock_network_reset();
    mock_system_reset();
    shell_register_berry_commands();
}

void tearDown(void) {}

static void test_run_resolves_relative_path_from_shell_directory(void)
{
    char *argv[] = {"berry", "run", "apps/demo.be"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("/flash/apps/demo.be", mock_berry_last_path());
    TEST_ASSERT_EQUAL(0, mock_berry_run_argc());
}

static void test_run_preserves_absolute_sdcard_path(void)
{
    char *argv[] = {"berry", "run", "/sdcard/berry/demo.be"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("/sdcard/berry/demo.be", mock_berry_last_path());
}

static void test_run_forwards_arguments(void)
{
    char *argv[] = {"berry", "run", "scripts/ping.be", "example.com", "two words", ""};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 6, argv));
    TEST_ASSERT_EQUAL_STRING("/flash/scripts/ping.be", mock_berry_last_path());
    TEST_ASSERT_EQUAL(3, mock_berry_run_argc());
    TEST_ASSERT_EQUAL_STRING("example.com", mock_berry_run_arg(0));
    TEST_ASSERT_EQUAL_STRING("two words", mock_berry_run_arg(1));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_run_arg(2));
}

static void test_run_returns_script_status(void)
{
    mock_berry_set_run_status(7);
    char *argv[] = {"berry", "run", "tool.be", "arg"};
    TEST_ASSERT_EQUAL(7, mock_console_run_cmd("berry", 4, argv));
}

static void test_run_runtime_error_fails_command(void)
{
    mock_berry_set_run_result(ESP_FAIL);
    char *argv[] = {"berry", "run", "tool.be"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
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
    char *argv[] = {"berry", "exec", "demo.be"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
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

static void test_autoexec_runs_startup_script(void)
{
    mock_filesystem_add_file("/flash/autoexec.be");
    TEST_ASSERT_EQUAL(ESP_OK, shell_berry_run_autoexec());
    TEST_ASSERT_EQUAL_STRING("/flash/autoexec.be", mock_berry_last_path());
}

static void test_autoexec_missing_script_is_not_an_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_berry_run_autoexec());
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_autoexec_execution_error_is_reported(void)
{
    mock_filesystem_add_file("/flash/autoexec.be");
    mock_berry_set_run_result(ESP_FAIL);
    TEST_ASSERT_EQUAL(ESP_FAIL, shell_berry_run_autoexec());
    TEST_ASSERT_EQUAL_STRING("/flash/autoexec.be", mock_berry_last_path());
}

static void test_autoexec_initialization_error_is_reported(void)
{
    mock_filesystem_add_file("/flash/autoexec.be");
    mock_berry_set_init_result(ESP_ERR_NO_MEM);
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, shell_berry_run_autoexec());
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_profile_prints_metrics(void)
{
    mock_system_set_free_heap(180000);
    mock_system_set_min_free_heap(160000);
    mock_system_set_max_alloc(110000);
    mock_berry_set_memory_info(true, 2048);
    char *argv[] = {"berry", "profile"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));
}

static void test_profile_accepts_heap_alias(void)
{
    char *argv[] = {"berry", "heap"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));
}

static void test_profile_rejects_extra_args(void)
{
    char *argv[] = {"berry", "profile", "extra"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
}

static void test_gc_collects_and_reports(void)
{
    mock_berry_set_gc_bytes(4096, 1024);
    char *argv[] = {"berry", "gc"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));
}

static void test_gc_reports_init_failure(void)
{
    mock_berry_set_init_result(ESP_ERR_NO_MEM);
    char *argv[] = {"berry", "gc"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 2, argv));
}

static void test_gc_reports_collect_failure(void)
{
    mock_berry_set_gc_collect_result(ESP_ERR_INVALID_STATE);
    char *argv[] = {"berry", "gc"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 2, argv));
}

static void test_gc_step_sets_rate(void)
{
    char *argv[] = {"berry", "gc-step", "150"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv));
}

static void test_gc_step_accepts_experiment_values(void)
{
    char *argv125[] = {"berry", "gc-step", "125"};
    char *argv200[] = {"berry", "gc-step", "200"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv125));
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 3, argv200));
}

static void test_gc_step_rejects_missing_arg(void)
{
    char *argv[] = {"berry", "gc-step"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 2, argv));
}

static void test_gc_step_rejects_invalid_rate(void)
{
    char *argv_abc[] = {"berry", "gc-step", "abc"};
    char *argv_low[] = {"berry", "gc-step", "50"};
    char *argv_high[] = {"berry", "gc-step", "501"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv_abc));
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv_low));
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv_high));
}

static void test_http_stack_reports_sample(void)
{
    char *argv[] = {"berry", "http-stack"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));
}

static void test_http_stack_accepts_net_stack_alias(void)
{
    char *argv[] = {"berry", "net-stack"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("berry", 2, argv));
}

static void test_http_stack_rejects_extra_args(void)
{
    char *argv[] = {"berry", "http-stack", "extra"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("berry", 3, argv));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_run_resolves_relative_path_from_shell_directory);
    RUN_TEST(test_run_preserves_absolute_sdcard_path);
    RUN_TEST(test_run_forwards_arguments);
    RUN_TEST(test_run_returns_script_status);
    RUN_TEST(test_run_runtime_error_fails_command);
    RUN_TEST(test_eval_joins_source_arguments);
    RUN_TEST(test_init_error_fails_command);
    RUN_TEST(test_execution_error_fails_command);
    RUN_TEST(test_unknown_operation_fails_command);
    RUN_TEST(test_missing_arguments_fails_command);
    RUN_TEST(test_close_stops_active_app);
    RUN_TEST(test_autoexec_runs_startup_script);
    RUN_TEST(test_autoexec_missing_script_is_not_an_error);
    RUN_TEST(test_autoexec_execution_error_is_reported);
    RUN_TEST(test_autoexec_initialization_error_is_reported);
    RUN_TEST(test_profile_prints_metrics);
    RUN_TEST(test_profile_accepts_heap_alias);
    RUN_TEST(test_profile_rejects_extra_args);
    RUN_TEST(test_gc_collects_and_reports);
    RUN_TEST(test_gc_reports_init_failure);
    RUN_TEST(test_gc_reports_collect_failure);
    RUN_TEST(test_gc_step_sets_rate);
    RUN_TEST(test_gc_step_accepts_experiment_values);
    RUN_TEST(test_gc_step_rejects_missing_arg);
    RUN_TEST(test_gc_step_rejects_invalid_rate);
    RUN_TEST(test_http_stack_reports_sample);
    RUN_TEST(test_http_stack_accepts_net_stack_alias);
    RUN_TEST(test_http_stack_rejects_extra_args);
    return UNITY_END();
}
