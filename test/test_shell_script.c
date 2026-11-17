#include "unity.h"

#include "filesystem.h"
#include "mock_berry.h"
#include "mock_console.h"
#include "mock_filesystem.h"
#include "mock_network.h"
#include "shell.h"
#include "shell_alias.h"
#include "shell_cmds.h"
#include "shell_redirection.h"
#include "shell_script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char s_captured_stdin[9000];
static size_t s_captured_stdin_len;
static int s_pipe_capture_calls;
static esp_err_t s_pipe_capture_result;
static esp_err_t s_pipe_capture_lhs_err;
static int s_pipe_capture_lhs_ret;
static char s_pipe_capture_last_lhs[128];

static esp_err_t test_pipe_capture_hook(const char *lhs, char **out_data, size_t *out_len, int *lhs_ret,
                                        esp_err_t *lhs_err)
{
    s_pipe_capture_calls++;
    if (lhs != NULL)
    {
        strncpy(s_pipe_capture_last_lhs, lhs, sizeof(s_pipe_capture_last_lhs) - 1);
        s_pipe_capture_last_lhs[sizeof(s_pipe_capture_last_lhs) - 1] = '\0';
    }
    else
    {
        s_pipe_capture_last_lhs[0] = '\0';
    }

    if (lhs_ret != NULL)
    {
        *lhs_ret = s_pipe_capture_lhs_ret;
    }
    if (lhs_err != NULL)
    {
        *lhs_err = s_pipe_capture_lhs_err;
    }
    if (s_pipe_capture_result != ESP_OK)
    {
        *out_data = NULL;
        *out_len = 0;
        return s_pipe_capture_result;
    }

    char *buf = (char *)malloc(s_captured_stdin_len + 1);
    if (buf == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    if (s_captured_stdin_len > 0)
    {
        memcpy(buf, s_captured_stdin, s_captured_stdin_len);
    }
    buf[s_captured_stdin_len] = '\0';
    *out_data = buf;
    *out_len = s_captured_stdin_len;
    return ESP_OK;
}

static void reset_pipe_capture_hook(void)
{
    shell_script_set_pipe_capture_for_test(NULL);
    memset(s_captured_stdin, 0, sizeof(s_captured_stdin));
    s_captured_stdin_len = 0;
    s_pipe_capture_calls = 0;
    s_pipe_capture_result = ESP_OK;
    s_pipe_capture_lhs_err = ESP_OK;
    s_pipe_capture_lhs_ret = 0;
    s_pipe_capture_last_lhs[0] = '\0';
}

static void set_pipe_capture_output(const char *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        s_captured_stdin[0] = '\0';
        s_captured_stdin_len = 0;
        return;
    }
    if (len > sizeof(s_captured_stdin))
    {
        len = sizeof(s_captured_stdin);
    }
    memcpy(s_captured_stdin, data, len);
    s_captured_stdin_len = len;
    s_captured_stdin[len < sizeof(s_captured_stdin) ? len : sizeof(s_captured_stdin) - 1] = '\0';
}

static void reset_shell_state(void)
{
    mock_console_reset();
    mock_filesystem_reset();
    mock_network_reset();
    mock_berry_reset();
    reset_pipe_capture_hook();
    shell_set_cwd("/flash");
    shell_register_fs_commands();
    shell_register_sd_commands();
    shell_register_info_commands();

    shell_register_berry_commands();
    TEST_ASSERT_EQUAL(ESP_OK, shell_alias_init());
    TEST_ASSERT_EQUAL(ESP_OK, shell_script_init());
}

void setUp(void)
{
    reset_shell_state();
}

void tearDown(void)
{
    reset_pipe_capture_hook();
}

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
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("./missing.be", &ret));
    TEST_ASSERT_EQUAL(1, ret);
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
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("bad", &ret));
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

static void test_path_remove_non_configured_returns_error(void)
{
    mock_filesystem_add_directory("/flash/scripts");
    char *add[] = {"path", "add", "/flash/scripts"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add));

    char *remove[] = {"path", "remove", "/flash/other"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 3, remove));

    char contents[128];
    size_t length = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_EQUAL_STRING("/flash/scripts\n", contents);
}

static void test_path_bad_arity_and_unknown_subcommand_return_error(void)
{
    char *bogus[] = {"path", "bogus"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 2, bogus));

    char *add_missing[] = {"path", "add"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 2, add_missing));

    char *remove_missing[] = {"path", "remove"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 2, remove_missing));

    char *clear_extra[] = {"path", "clear", "extra"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 3, clear_extra));
}

static void test_path_max_eight_directories(void)
{
    char name[VFS_PATH_MAX];
    for (int i = 0; i < 8; i++)
    {
        snprintf(name, sizeof(name), "/flash/d%d", i);
        mock_filesystem_add_directory(name);
        char *add[] = {"path", "add", name};
        TEST_ASSERT_EQUAL_MESSAGE(0, mock_console_run_cmd("path", 3, add), name);
    }

    mock_filesystem_add_directory("/flash/d8");
    char *add_ninth[] = {"path", "add", "/flash/d8"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("path", 3, add_ninth));

    char contents[512];
    size_t length = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file("/flash/.cos_path", contents, sizeof(contents) - 1, &length));
    contents[length] = '\0';
    TEST_ASSERT_NOT_NULL(strstr(contents, "/flash/d0\n"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "/flash/d7\n"));
    TEST_ASSERT_NULL(strstr(contents, "/flash/d8"));
}

static void test_explicit_path_to_directory_reports_error(void)
{
    mock_filesystem_add_directory("/flash/tools");
    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("/flash/tools", &ret));
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("", mock_berry_last_path());
}

static void test_path_list_and_no_args_succeed(void)
{
    char *list[] = {"path", "list"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 2, list));
    char *plain[] = {"path"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 1, plain));
}

static void test_redirection_parse_none_for_plain_command(void)
{
    shell_redirection_t redir;
    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("echo hello world", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_NONE, redir.type);
}

static void test_redirection_parse_truncate_and_append(void)
{
    shell_redirection_t redir;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("echo hi > /sdcard/out.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_TRUNCATE, redir.type);
    TEST_ASSERT_EQUAL_STRING("echo hi", redir.command);
    TEST_ASSERT_EQUAL_STRING("/sdcard/out.txt", redir.target);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("echo hi>>out.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_APPEND, redir.type);
    TEST_ASSERT_EQUAL_STRING("echo hi", redir.command);
    TEST_ASSERT_EQUAL_STRING("out.txt", redir.target);
}

static void test_redirection_parse_quoted_target_and_quoted_operator_ignored(void)
{
    shell_redirection_t redir;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("ls > \"my file.txt\"", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_TRUNCATE, redir.type);
    TEST_ASSERT_EQUAL_STRING("ls", redir.command);
    TEST_ASSERT_EQUAL_STRING("my file.txt", redir.target);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("echo \"a > b\"", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_NONE, redir.type);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("echo 'x >> y' >> real.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_APPEND, redir.type);
    TEST_ASSERT_EQUAL_STRING("echo 'x >> y'", redir.command);
    TEST_ASSERT_EQUAL_STRING("real.txt", redir.target);
}

static void test_redirection_parse_rejects_malformed_or_unsupported_forms(void)
{
    shell_redirection_t redir;

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_parse_redirection("> out.txt", &redir));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_parse_redirection("echo hi >", &redir));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_parse_redirection("echo hi > a > b", &redir));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_parse_redirection("echo hi >>> out.txt", &redir));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_parse_redirection("echo hi > \"unterminated", &redir));
}

static void test_dispatch_reports_invalid_redirection_syntax(void)
{
    int ret = -1;

    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("info >", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("echo hi > a > b", &ret));
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_dispatch_rejects_redirection_target_directory(void)
{
    mock_filesystem_add_directory("/flash/outdir");

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("ls > /flash/outdir", &ret));
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_pipeline_parse_none_for_plain_or_quoted_pipe(void)
{
    shell_pipeline_info_t info;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("echo hi", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_NONE, info.status);
    TEST_ASSERT_EQUAL(0, info.segment_count);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("echo \"a|b\"", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_NONE, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("echo 'x|y'", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_NONE, info.status);
}

static void test_pipeline_parse_detects_unsupported_pipelines(void)
{
    shell_pipeline_info_t info;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("cat a.txt | grep b", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_UNSUPPORTED, info.status);
    TEST_ASSERT_EQUAL(2, info.segment_count);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("cat a.txt|grep b", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_UNSUPPORTED, info.status);
    TEST_ASSERT_EQUAL(2, info.segment_count);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("a | b | c", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_UNSUPPORTED, info.status);
    TEST_ASSERT_EQUAL(3, info.segment_count);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("a | b > out.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_UNSUPPORTED, info.status);
    TEST_ASSERT_EQUAL(2, info.segment_count);
}

static void test_pipeline_parse_rejects_invalid_syntax(void)
{
    shell_pipeline_info_t info;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("| a", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_INVALID, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("a |", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_INVALID, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("a |   ", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_INVALID, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_pipeline("a || b", &info));
    TEST_ASSERT_EQUAL(SHELL_PIPELINE_INVALID, info.status);
}

static void test_dispatch_reports_unsupported_and_invalid_pipes(void)
{
    int ret = -1;

    /* Pipe + redirection combinations remain unsupported in v1. */
    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("echo hi | cat > /flash/out.txt", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("| echo hi", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("echo hi |", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("echo hi || cat", &ret));
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_pipeline_split_two_and_combo_helpers(void)
{
    char lhs[64];
    char rhs[64];

    TEST_ASSERT_EQUAL(ESP_OK, shell_pipeline_split_two("echo hi | consumer.be", lhs, sizeof(lhs), rhs, sizeof(rhs)));
    TEST_ASSERT_EQUAL_STRING("echo hi", lhs);
    TEST_ASSERT_EQUAL_STRING("consumer.be", rhs);

    TEST_ASSERT_TRUE(shell_pipeline_has_redirection_combo("echo hi | cat > /flash/out.txt"));
    TEST_ASSERT_TRUE(shell_pipeline_has_redirection_combo("echo hi 2> /tmp/e | cat"));
    TEST_ASSERT_FALSE(shell_pipeline_has_redirection_combo("echo \"a > b\" | consumer.be"));
}

static void test_dispatch_pipe_v1_unsupported_consumer_diagnostics(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    set_pipe_capture_output("hello\n", 6);
    s_pipe_capture_lhs_err = ESP_OK;
    s_pipe_capture_lhs_ret = 0;

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("producer | cat", &ret));
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL(0, mock_berry_run_call_count());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());

    /* 3+ stage pipelines remain unsupported. */
    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("a | b | c", &ret));
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
}

static void test_dispatch_pipe_v1_runs_berry_rhs_and_clears_stdin(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    set_pipe_capture_output("payload-123", 11);
    s_pipe_capture_lhs_err = ESP_OK;
    s_pipe_capture_lhs_ret = 0;
    mock_berry_set_run_result(ESP_OK);
    mock_berry_set_run_status(7);

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("producer | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(7, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL_STRING("producer", s_pipe_capture_last_lhs);
    TEST_ASSERT_EQUAL(1, mock_berry_run_call_count());
    TEST_ASSERT_EQUAL_STRING("/flash/rhs.be", mock_berry_last_path());
    TEST_ASSERT_TRUE(mock_berry_last_piped_stdin_set_seen());
    TEST_ASSERT_EQUAL_STRING("payload-123", mock_berry_last_piped_stdin_seen());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());
    TEST_ASSERT_EQUAL_STRING("", mock_berry_piped_stdin());
}

static void test_dispatch_pipe_v1_returns_rhs_status_when_lhs_nonzero(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    set_pipe_capture_output("ok", 2);
    s_pipe_capture_lhs_err = ESP_OK;
    s_pipe_capture_lhs_ret = 3;
    mock_berry_set_run_result(ESP_OK);
    mock_berry_set_run_status(0);

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("producer | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_berry_run_call_count());
    TEST_ASSERT_TRUE(mock_berry_last_piped_stdin_set_seen());
    TEST_ASSERT_EQUAL_STRING("ok", mock_berry_last_piped_stdin_seen());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());
}

static void test_dispatch_pipe_v1_overflow_capture_fails_without_rhs(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    s_pipe_capture_result = ESP_ERR_INVALID_SIZE;

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("producer | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL(0, mock_berry_run_call_count());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());
}

static void test_dispatch_pipe_v1_capture_io_failure_without_rhs(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    s_pipe_capture_result = ESP_FAIL;

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("producer | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL(0, mock_berry_run_call_count());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());
}

static void test_dispatch_pipe_v1_lhs_dispatch_error_not_reported_as_capture_failure(void)
{
    mock_filesystem_add_file("/flash/rhs.be");

    shell_script_set_pipe_capture_for_test(test_pipe_capture_hook);
    set_pipe_capture_output("", 0);
    s_pipe_capture_lhs_err = ESP_ERR_NOT_FOUND;
    s_pipe_capture_lhs_ret = 0;

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("missingcmd | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(127, ret);
    TEST_ASSERT_EQUAL(1, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL(0, mock_berry_run_call_count());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());

    /* Generic lhs dispatch error preserves failure and does not run rhs. */
    s_pipe_capture_lhs_err = ESP_FAIL;
    s_pipe_capture_lhs_ret = 2;
    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("badlhs | /flash/rhs.be", &ret));
    TEST_ASSERT_EQUAL(2, ret);
    TEST_ASSERT_EQUAL(2, s_pipe_capture_calls);
    TEST_ASSERT_EQUAL(0, mock_berry_run_call_count());
    TEST_ASSERT_FALSE(mock_berry_piped_stdin_set());
}

static void test_fd_redirection_parse_none_for_plain_stdout_and_quoted_forms(void)
{
    shell_fd_redirect_info_t info;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("info > out.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_NONE, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("info >> out.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_NONE, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("echo \"2> err.txt\"", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_NONE, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("echo '&> all.txt'", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_NONE, info.status);
}

static void test_fd_redirection_parse_detects_stderr_and_combined_forms(void)
{
    shell_fd_redirect_info_t info;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("cmd 2> err.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_UNSUPPORTED, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("cmd 2>>err.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_UNSUPPORTED, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("cmd &> all.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_UNSUPPORTED, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("cmd &>>all.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_UNSUPPORTED, info.status);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_fd_redirection("cmd 2> err.txt > out.txt", &info));
    TEST_ASSERT_EQUAL(SHELL_FD_REDIRECT_UNSUPPORTED, info.status);
}

static void test_redirection_parse_stderr_and_combined_forms(void)
{
    shell_redirection_t redir;

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("cmd 2> err.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_STDERR_TRUNCATE, redir.type);
    TEST_ASSERT_EQUAL_STRING("cmd", redir.command);
    TEST_ASSERT_EQUAL_STRING("err.txt", redir.target);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("cmd 2>> \"err.log\"", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_STDERR_APPEND, redir.type);
    TEST_ASSERT_EQUAL_STRING("err.log", redir.target);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("cmd &> all.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_BOTH_TRUNCATE, redir.type);
    TEST_ASSERT_EQUAL_STRING("all.txt", redir.target);

    TEST_ASSERT_EQUAL(ESP_OK, shell_parse_redirection("cmd &>>all.txt", &redir));
    TEST_ASSERT_EQUAL(SHELL_REDIRECT_BOTH_APPEND, redir.type);
    TEST_ASSERT_EQUAL_STRING("all.txt", redir.target);
}

static void test_dispatch_handles_stderr_fd_redirection_runtime_path(void)
{
    int ret = -1;

    /* Best-effort runtime redirection is attempted (not rejected as unsupported).
     * In this host test environment the target fopen may fail; dispatch must still
     * handle the line and report non-zero status without mis-executing as plain args. */
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("info 2> /flash/err.txt", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("info 2>> /flash/err.txt", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("info &> /flash/all.txt", &ret));
    TEST_ASSERT_EQUAL(1, ret);

    ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("info &>> /flash/all.txt", &ret));
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_bare_command_searches_default_flash_scripts(void)
{
    mock_filesystem_add_directory("/flash/scripts");
    mock_filesystem_add_file("/flash/scripts/tool.be");

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/scripts/tool.be", mock_berry_last_path());
}

static void test_configured_path_wins_over_default_flash_scripts(void)
{
    mock_filesystem_add_directory("/flash/scripts");
    mock_filesystem_add_directory("/flash/custom");
    mock_filesystem_add_file("/flash/scripts/tool.be");
    mock_filesystem_add_file("/flash/custom/tool.be");

    char *add[] = {"path", "add", "/flash/custom"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("path", 3, add));

    int ret = -1;
    TEST_ASSERT_EQUAL(ESP_OK, shell_dispatch_expanded_line("tool", &ret));
    TEST_ASSERT_EQUAL_STRING("/flash/custom/tool.be", mock_berry_last_path());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_path_command_lists_adds_removes_and_persists);
    RUN_TEST(test_path_add_rejects_unavailable_directory);
    RUN_TEST(test_path_clear_persists_empty_list);
    RUN_TEST(test_path_remove_non_configured_returns_error);
    RUN_TEST(test_path_bad_arity_and_unknown_subcommand_return_error);
    RUN_TEST(test_path_max_eight_directories);
    RUN_TEST(test_path_list_and_no_args_succeed);
    RUN_TEST(test_bare_command_runs_script_from_cwd);
    RUN_TEST(test_bare_command_searches_configured_path_in_order);
    RUN_TEST(test_cwd_script_wins_over_path);
    RUN_TEST(test_native_command_wins_over_script);
    RUN_TEST(test_explicit_relative_script_path_runs_only_that_script);
    RUN_TEST(test_explicit_missing_script_reports_path_error);
    RUN_TEST(test_explicit_path_to_directory_reports_error);
    RUN_TEST(test_quoted_arguments_preserved_for_script);
    RUN_TEST(test_script_status_propagated);
    RUN_TEST(test_script_runtime_error_reported);
    RUN_TEST(test_unknown_command_without_script_still_not_found);
    RUN_TEST(test_command_name_with_be_extension_resolves);
    RUN_TEST(test_redirection_parse_none_for_plain_command);
    RUN_TEST(test_redirection_parse_truncate_and_append);
    RUN_TEST(test_redirection_parse_quoted_target_and_quoted_operator_ignored);
    RUN_TEST(test_redirection_parse_rejects_malformed_or_unsupported_forms);
    RUN_TEST(test_dispatch_reports_invalid_redirection_syntax);
    RUN_TEST(test_dispatch_rejects_redirection_target_directory);
    RUN_TEST(test_pipeline_parse_none_for_plain_or_quoted_pipe);
    RUN_TEST(test_pipeline_parse_detects_unsupported_pipelines);
    RUN_TEST(test_pipeline_parse_rejects_invalid_syntax);
    RUN_TEST(test_dispatch_reports_unsupported_and_invalid_pipes);
    RUN_TEST(test_pipeline_split_two_and_combo_helpers);
    RUN_TEST(test_dispatch_pipe_v1_unsupported_consumer_diagnostics);
    RUN_TEST(test_dispatch_pipe_v1_runs_berry_rhs_and_clears_stdin);
    RUN_TEST(test_dispatch_pipe_v1_returns_rhs_status_when_lhs_nonzero);
    RUN_TEST(test_dispatch_pipe_v1_overflow_capture_fails_without_rhs);
    RUN_TEST(test_dispatch_pipe_v1_capture_io_failure_without_rhs);
    RUN_TEST(test_dispatch_pipe_v1_lhs_dispatch_error_not_reported_as_capture_failure);
    RUN_TEST(test_fd_redirection_parse_none_for_plain_stdout_and_quoted_forms);
    RUN_TEST(test_fd_redirection_parse_detects_stderr_and_combined_forms);
    RUN_TEST(test_redirection_parse_stderr_and_combined_forms);
    RUN_TEST(test_dispatch_handles_stderr_fd_redirection_runtime_path);
    RUN_TEST(test_bare_command_searches_default_flash_scripts);
    RUN_TEST(test_configured_path_wins_over_default_flash_scripts);
    return UNITY_END();
}
