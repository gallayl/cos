#include "unity.h"

#include "filesystem.h"
#include "mock_filesystem.h"

void setUp(void)
{
    mock_filesystem_reset();
}

void tearDown(void)
{
}

/* --- vfs_resolve_path tests --- */

static void test_resolve_flash_root(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, vfs_resolve_path("/flash", real, sizeof(real)));
    TEST_ASSERT_EQUAL_STRING("/littlefs/", real);
}

static void test_resolve_flash_subpath(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, vfs_resolve_path("/flash/data/file.txt", real, sizeof(real)));
    TEST_ASSERT_EQUAL_STRING("/littlefs/data/file.txt", real);
}

static void test_resolve_flash_trailing_slash(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, vfs_resolve_path("/flash/", real, sizeof(real)));
    TEST_ASSERT_EQUAL_STRING("/littlefs/", real);
}

static void test_resolve_sdcard_root(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, vfs_resolve_path("/sdcard", real, sizeof(real)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/", real);
}

static void test_resolve_sdcard_subpath(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_OK, vfs_resolve_path("/sdcard/photos/img.jpg", real, sizeof(real)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/photos/img.jpg", real);
}

static void test_resolve_invalid_prefix(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, vfs_resolve_path("/unknown/path", real, sizeof(real)));
}

static void test_resolve_null_args(void)
{
    char real[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_resolve_path(NULL, real, sizeof(real)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_resolve_path("/flash", NULL, sizeof(real)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_resolve_path("/flash", real, 0));
}

static void test_resolve_buffer_too_small(void)
{
    char real[4];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, vfs_resolve_path("/flash/longpath", real, sizeof(real)));
}

/* --- vfs_exists tests --- */

static void test_exists_root(void)
{
    TEST_ASSERT_TRUE(vfs_exists("/"));
}

static void test_exists_flash(void)
{
    TEST_ASSERT_TRUE(vfs_exists("/flash"));
}

static void test_exists_sdcard_not_mounted(void)
{
    TEST_ASSERT_FALSE(vfs_exists("/sdcard"));
}

static void test_exists_sdcard_mounted(void)
{
    mock_filesystem_set_sd_mounted(true);
    TEST_ASSERT_TRUE(vfs_exists("/sdcard"));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_resolve_flash_root);
    RUN_TEST(test_resolve_flash_subpath);
    RUN_TEST(test_resolve_flash_trailing_slash);
    RUN_TEST(test_resolve_sdcard_root);
    RUN_TEST(test_resolve_sdcard_subpath);
    RUN_TEST(test_resolve_invalid_prefix);
    RUN_TEST(test_resolve_null_args);
    RUN_TEST(test_resolve_buffer_too_small);
    RUN_TEST(test_exists_root);
    RUN_TEST(test_exists_flash);
    RUN_TEST(test_exists_sdcard_not_mounted);
    RUN_TEST(test_exists_sdcard_mounted);

    return UNITY_END();
}
