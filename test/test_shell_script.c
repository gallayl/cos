#include "unity.h"

#include "filesystem.h"
#include "mock_berry.h"
#include "mock_console.h"
#include "mock_filesystem.h"
#include "mock_network.h"
#include "shell.h"
#include "shell_alias.h"
#include "shell_cmds.h"
#include "shell_script.h"

#include <stdio.h>
#include <string.h>

static void reset_shell_state(void)
{
    mock_console_reset();
    mock_filesystem_reset();
    mock_network_reset();
    mock_berry_reset();
    shell_set_cwd("/flash");
    shell_register_fs_commands();
    shell_register_sd_commands();
    shell_register_info_commands();
    shell_register_network_commands();
    shell_register_berry_commands();
    TEST_ASSERT_EQUAL(ESP_OK, shell_alias_init());
    TEST_ASSERT_EQUAL(ESP_OK, shell_script_init());
}

void setUp(void)
{
    reset_shell_state();
}

void tearDown(void) {}

static void test_path_command_lists_adds_removes_and_persists(void)
{
    char *add[] = {"path", "add", "/flash/scripts"};
    mock_filesystem_add_directory("/flash/scripts");
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add));

    char contents[128];
    size_t length = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_EQUAL_STRING("/flash/scripts\n", contents);

    char *add_dup[] = {"path", "add", "scripts"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add_dup));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_EQUAL_STRING("/flash/scripts\n", contents);

    /* Simulate reboot: keep filesystem, reinitialize shell command/path state. */
    mock_console_reset();
    mock_berry_reset();
    shell_set_cwd("/flash");
    shell_register_fs_commands();
    shell_register_sd_commands();
    shell_register_info_commands();
    shell_register_network_commands();
    shell_register_berry_commands();
    TEST_ASSERT_EQUAL(ESP_OK, shell_alias_init());
    TEST_ASSERT_EQUAL(ESP_OK, shell_script_init());

    char *remove[] = {"path", "remove", "/flash/scripts"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, remove));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_EQUAL_STRING("", contents);
}

static void test_path_add_rejects_unavailable_directory(void)
{
    char *add[] = {"path", "add", "/flash/missing"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 3, add));
    TEST_ASSERT_FALSE(vfs_exists("/flash/.cos_path"));
}

static void test_path_clear_persists_empty_list(void)
{
    mock_filesystem_add_directory("/flash/scripts");
    char *add[] = {"path", "add", "/flash/scripts"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add));
    char *clear[] = {"path", "clear"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 2, clear));

    char contents[16];
    size_t length = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_EQUAL_STRING("", contents);
}

static void test_bare_command_runs_script_from_cwd(void)
{
    mock_filesystem_add_file("/flash/hello.be");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("hello", &ret));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("/flash/hello.be", mock_berry_last_path());
}

static void test_bare_command_searches_configured_path_in_order(void)
{
    mock_filesystem_add_directory("/flash/a");
    mock_filesystem_add_directory("/flash/b");
    mock_filesystem_add_file("/flash/b/tool.be");
    char *add_a[] = {"path", "add", "/flash/a"};
    char *add_b[] = {"path", "add", "/flash/b"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add_a));
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add_b));

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/b/tool.be", mock_berry_last_path());

    mock_filesystem_add_file("/flash/a/tool.be");
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/a/tool.be", mock_berry_last_path());
}

static void test_cwd_script_wins_over_path(void)
{
    mock_filesystem_add_directory("/flash/scripts");
    mock_filesystem_add_file("/flash/scripts/tool.be");
    char *add[] = {"path", "add", "/flash/scripts"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add));
    mock_filesystem_add_file("/flash/tool.be");

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/tool.be", mock_berry_last_path());
}

static void test_native_command_wins_over_script(void)
{
    mock_filesystem_add_file("/flash/ls.be");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("ls /flash", &ret));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_explicit_relative_script_path_runs_only_that_script(void)
{
    mock_filesystem_add_directory("/flash/tools");
    mock_filesystem_add_file("/flash/tools/ping.be");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tools/ping.be example.com 500", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/tools/ping.be", mock_berry_last_path());
    TEST_ASSERT_EQUAL(2, mock_berry_exec_argc());
    TEST_ASSERT_EQUAL_STRING("example.com", mock_berry_exec_arg(0));
    TEST_ASSERT_EQUAL_STRING("500", mock_berry_exec_arg(1));
}

static void test_explicit_missing_script_reports_path_error(void)
{
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, shell_dispatch_expanded_line("./missing.be", &ret));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_quoted_arguments_preserved_for_script(void)
{
    mock_filesystem_add_file("/flash/echoargs.be");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("echoargs \"two words\" plain", &ret));
    TEST_ASSERT_EQUAL(2, mock_berry_exec_argc());
    TEST_ASSERT_EQUAL_STRING("two words", mock_berry_exec_arg(0));
    TEST_ASSERT_EQUAL_STRING("plain", mock_berry_exec_arg(1));
}

static void test_script_status_propagated(void)
{
    mock_filesystem_add_file("/flash/fail.be");
    mock_berry_set_exec_status(7);
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("fail", &ret));
    TEST_ASSERT_EQUAL(7, ret);
}

static void test_script_runtime_error_reported(void)
{
    mock_filesystem_add_file("/flash/bad.be");
    mock_berry_set_exec_result(ESP_FAIL);
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_FAIL, shell_dispatch_expanded_line("bad", &ret));
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_unknown_command_without_script_still_not_found(void)
{
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, shell_dispatch_expanded_line("doesnotexist", &ret));
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_command_name_with_be_extension_resolves(void)
{
    mock_filesystem_add_file("/flash/tool.be");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool.be", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/tool.be", mock_berry_last_path());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_path_command_lists_adds_removes_and_persists);
    RUN_TEST(test_path_add_rejects_unavailable_directory);
    RUN_TEST(test_path_clear_persists_empty_list);
    RUN_TEST(test_bare_command_runs_script_from_cwd);
    RUN_TEST(test_bare_command_searches_configured_path_in_order);
    RUN_TEST(test_cwd_script_wins_over_path);
    RUN_TEST(test_native_command_wins_over_script);
    RUN_TEST(test_explicit_relative_script_path_runs_only_that_script);
    RUN_TEST(test_explicit_missing_script_reports_path_error);
    RUN_TEST(test_quoted_arguments_preserved_for_script);
    RUN_TEST(test_script_status_propagated);
    RUN_TEST(test_script_runtime_error_reported);
    RUN_TEST(test_unknown_command_without_script_still_not_found);
    RUN_TEST(test_command_name_with_be_extension_resolves);
    return UNITY_END();
}
