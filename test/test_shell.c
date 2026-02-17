#include "unity.h"

#include "filesystem.h"
#include "mock_filesystem.h"
#include "shell.h"

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
    /* cwd should remain unchanged */
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

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_resolve_absolute_path);
    RUN_TEST(test_resolve_relative_from_flash);
    RUN_TEST(test_resolve_dot);
    RUN_TEST(test_resolve_dotdot_from_subdir);
    RUN_TEST(test_resolve_dotdot_from_root);
    RUN_TEST(test_resolve_dotdot_past_root);
    RUN_TEST(test_resolve_complex_path);
    RUN_TEST(test_resolve_null_args);
    RUN_TEST(test_set_cwd_flash);
    RUN_TEST(test_set_cwd_invalid);
    RUN_TEST(test_set_cwd_strips_trailing_slash);
    RUN_TEST(test_prompt_flash_root);
    RUN_TEST(test_prompt_root);
    RUN_TEST(test_prompt_sdcard);
    RUN_TEST(test_prompt_subdir);

    return UNITY_END();
}
