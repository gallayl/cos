#include "unity.h"

#include "filesystem.h"
#include "mock_console.h"
#include "mock_filesystem.h"
#include "shell.h"
#include "shell_cmds.h"

void setUp(void)
{
    mock_filesystem_reset();
    shell_set_cwd("/flash");
}

void tearDown(void)
{
}

/* --- shell_resolve_relative tests --- */

static void test_resolve_absolute_path(void)
{
    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("/sdcard/data", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/data", abs);
}

static void test_resolve_relative_from_flash(void)
{
    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("data", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/flash/data", abs);
}

static void test_resolve_dot(void)
{
    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative(".", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/flash", abs);
}

static void test_resolve_dotdot_from_subdir(void)
{
    mock_filesystem_add_directory("/flash/data");
    shell_set_cwd("/flash/data");

    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("..", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/flash", abs);
}

static void test_resolve_dotdot_from_root(void)
{
    shell_set_cwd("/flash");

    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("..", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/", abs);
}

static void test_resolve_dotdot_past_root(void)
{
    shell_set_cwd("/flash");

    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("../..", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/", abs);
}

static void test_resolve_complex_path(void)
{
    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, shell_resolve_relative("/flash/a/b/../c/./d", abs, sizeof(abs)));
    TEST_ASSERT_EQUAL_STRING("/flash/a/c/d", abs);
}

static void test_resolve_null_args(void)
{
    char abs[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_resolve_relative(NULL, abs, sizeof(abs)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_resolve_relative("foo", NULL, sizeof(abs)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, shell_resolve_relative("foo", abs, 0));
}

/* --- shell_set_cwd / shell_get_cwd tests --- */

static void test_set_cwd_flash(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_set_cwd("/flash"));
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_set_cwd_invalid(void)
{
    TEST_ASSERT_NOT_EQUAL(ESP_OK, shell_set_cwd("/nonexistent"));
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_set_cwd_strips_trailing_slash(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, shell_set_cwd("/flash"));
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

/* --- prompt tests --- */

static void test_prompt_flash_root(void)
{
    shell_set_cwd("/flash");
    TEST_ASSERT_EQUAL_STRING("COS/flash> ", shell_get_prompt());
}

static void test_prompt_root(void)
{
    shell_set_cwd("/");
    TEST_ASSERT_EQUAL_STRING("COS> ", shell_get_prompt());
}

static void test_prompt_sdcard(void)
{
    mock_filesystem_set_sd_mounted(true);
    shell_set_cwd("/sdcard");
    TEST_ASSERT_EQUAL_STRING("COS/sdcard> ", shell_get_prompt());
}

static void test_prompt_subdir(void)
{
    mock_filesystem_add_directory("/flash/data");
    shell_set_cwd("/flash/data");
    TEST_ASSERT_EQUAL_STRING("COS/flash/data> ", shell_get_prompt());
}

/* --- command handler tests --- */

static void test_cmd_cd_to_flash(void)
{
    shell_set_cwd("/");
    char *argv[] = {"cd", "/flash"};
    int ret = mock_console_run_cmd("cd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_cmd_cd_no_arg_defaults_to_flash(void)
{
    mock_filesystem_add_directory("/flash/data");
    shell_set_cwd("/flash/data");
    char *argv[] = {"cd"};
    int ret = mock_console_run_cmd("cd", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_cmd_cd_invalid_dir(void)
{
    char *argv[] = {"cd", "/nonexistent"};
    int ret = mock_console_run_cmd("cd", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_cmd_cd_relative(void)
{
    mock_filesystem_add_directory("/flash/data");
    char *argv[] = {"cd", "data"};
    int ret = mock_console_run_cmd("cd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("/flash/data", shell_get_cwd());
}

static void test_cmd_cd_dotdot(void)
{
    mock_filesystem_add_directory("/flash/data");
    shell_set_cwd("/flash/data");
    char *argv[] = {"cd", ".."};
    int ret = mock_console_run_cmd("cd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("/flash", shell_get_cwd());
}

static void test_cmd_pwd(void)
{
    char *argv[] = {"pwd"};
    int ret = mock_console_run_cmd("pwd", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_ls_cwd(void)
{
    char *argv[] = {"ls"};
    int ret = mock_console_run_cmd("ls", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_ls_explicit_path(void)
{
    char *argv[] = {"ls", "/flash"};
    int ret = mock_console_run_cmd("ls", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_mkdir(void)
{
    char *argv[] = {"mkdir", "newdir"};
    int ret = mock_console_run_cmd("mkdir", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_mkdir_no_arg(void)
{
    char *argv[] = {"mkdir"};
    int ret = mock_console_run_cmd("mkdir", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_rm(void)
{
    char *argv[] = {"rm", "somefile"};
    int ret = mock_console_run_cmd("rm", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_rm_no_arg(void)
{
    char *argv[] = {"rm"};
    int ret = mock_console_run_cmd("rm", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_cat_no_arg(void)
{
    char *argv[] = {"cat"};
    int ret = mock_console_run_cmd("cat", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_touch(void)
{
    char *argv[] = {"touch", "newfile.txt"};
    int ret = mock_console_run_cmd("touch", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_touch_no_arg(void)
{
    char *argv[] = {"touch"};
    int ret = mock_console_run_cmd("touch", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_hexdump_no_arg(void)
{
    char *argv[] = {"hexdump"};
    int ret = mock_console_run_cmd("hexdump", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_sd_no_arg(void)
{
    char *argv[] = {"sd"};
    int ret = mock_console_run_cmd("sd", 1, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_sd_mount(void)
{
    char *argv[] = {"sd", "mount"};
    int ret = mock_console_run_cmd("sd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_TRUE(sdcard_is_mounted());
}

static void test_cmd_sd_unmount(void)
{
    mock_filesystem_set_sd_mounted(true);
    char *argv[] = {"sd", "unmount"};
    int ret = mock_console_run_cmd("sd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_FALSE(sdcard_is_mounted());
}

static void test_cmd_sd_info_not_mounted(void)
{
    char *argv[] = {"sd", "info"};
    int ret = mock_console_run_cmd("sd", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_sd_info_mounted(void)
{
    mock_filesystem_set_sd_mounted(true);
    char *argv[] = {"sd", "info"};
    int ret = mock_console_run_cmd("sd", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_sd_unknown_subcommand(void)
{
    char *argv[] = {"sd", "bogus"};
    int ret = mock_console_run_cmd("sd", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_cmd_info(void)
{
    char *argv[] = {"info"};
    int ret = mock_console_run_cmd("info", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_cmd_format(void)
{
    char *argv[] = {"format"};
    int ret = mock_console_run_cmd("format", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

int main(void)
{
    mock_console_reset();
    shell_register_fs_commands();
    shell_register_sd_commands();
    shell_register_info_commands();

    UNITY_BEGIN();

    /* resolve_relative */
    RUN_TEST(test_resolve_absolute_path);
    RUN_TEST(test_resolve_relative_from_flash);
    RUN_TEST(test_resolve_dot);
    RUN_TEST(test_resolve_dotdot_from_subdir);
    RUN_TEST(test_resolve_dotdot_from_root);
    RUN_TEST(test_resolve_dotdot_past_root);
    RUN_TEST(test_resolve_complex_path);
    RUN_TEST(test_resolve_null_args);

    /* set_cwd / get_cwd */
    RUN_TEST(test_set_cwd_flash);
    RUN_TEST(test_set_cwd_invalid);
    RUN_TEST(test_set_cwd_strips_trailing_slash);

    /* prompt */
    RUN_TEST(test_prompt_flash_root);
    RUN_TEST(test_prompt_root);
    RUN_TEST(test_prompt_sdcard);
    RUN_TEST(test_prompt_subdir);

    /* fs commands */
    RUN_TEST(test_cmd_cd_to_flash);
    RUN_TEST(test_cmd_cd_no_arg_defaults_to_flash);
    RUN_TEST(test_cmd_cd_invalid_dir);
    RUN_TEST(test_cmd_cd_relative);
    RUN_TEST(test_cmd_cd_dotdot);
    RUN_TEST(test_cmd_pwd);
    RUN_TEST(test_cmd_ls_cwd);
    RUN_TEST(test_cmd_ls_explicit_path);
    RUN_TEST(test_cmd_mkdir);
    RUN_TEST(test_cmd_mkdir_no_arg);
    RUN_TEST(test_cmd_rm);
    RUN_TEST(test_cmd_rm_no_arg);
    RUN_TEST(test_cmd_cat_no_arg);
    RUN_TEST(test_cmd_touch);
    RUN_TEST(test_cmd_touch_no_arg);
    RUN_TEST(test_cmd_hexdump_no_arg);

    /* sd commands */
    RUN_TEST(test_cmd_sd_no_arg);
    RUN_TEST(test_cmd_sd_mount);
    RUN_TEST(test_cmd_sd_unmount);
    RUN_TEST(test_cmd_sd_info_not_mounted);
    RUN_TEST(test_cmd_sd_info_mounted);
    RUN_TEST(test_cmd_sd_unknown_subcommand);

    /* info commands */
    RUN_TEST(test_cmd_info);
    RUN_TEST(test_cmd_format);

    return UNITY_END();
}
