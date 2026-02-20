#include "unity.h"

#include "filesystem.h"
#include "nvs_flash.h"

#include <stdlib.h>
#include <string.h>

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/* ── Write / Read round-trip ──────────────────────────────────────────── */

TEST_CASE("write and read file round-trip", "[vfs]")
{
    const char *path = "/flash/roundtrip.txt";
    const char *data = "Integration test payload 12345";
    size_t data_len = strlen(data);

    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, data, data_len));

    char buf[128];
    size_t bytes_read = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file(path, buf, sizeof(buf), &bytes_read));
    TEST_ASSERT_EQUAL(data_len, bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(data, buf, data_len);
}

TEST_CASE("overwrite replaces content", "[vfs]")
{
    const char *path = "/flash/overwrite.txt";

    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, "first", 5));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, "second", 6));

    char buf[64];
    size_t bytes_read = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file(path, buf, sizeof(buf), &bytes_read));
    TEST_ASSERT_EQUAL(6, bytes_read);
    TEST_ASSERT_EQUAL_MEMORY("second", buf, 6);
}

TEST_CASE("write empty file", "[vfs]")
{
    const char *path = "/flash/empty.txt";

    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, "", 0));
    TEST_ASSERT_TRUE(vfs_exists(path));

    char buf[16];
    size_t bytes_read = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_read_file(path, buf, sizeof(buf), &bytes_read));
    TEST_ASSERT_EQUAL(0, bytes_read);
}

/* ── vfs_exists / vfs_is_directory ────────────────────────────────────── */

TEST_CASE("exists returns true for file", "[vfs]")
{
    const char *path = "/flash/exists_file.txt";
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, "x", 1));
    TEST_ASSERT_TRUE(vfs_exists(path));
    TEST_ASSERT_FALSE(vfs_is_directory(path));
}

TEST_CASE("exists returns false for absent path", "[vfs]")
{
    TEST_ASSERT_FALSE(vfs_exists("/flash/no_such_thing.dat"));
}

TEST_CASE("is_directory returns true for directory", "[vfs]")
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/adir"));
    TEST_ASSERT_TRUE(vfs_exists("/flash/adir"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash/adir"));
}

TEST_CASE("flash root is a directory", "[vfs]")
{
    TEST_ASSERT_TRUE(vfs_exists("/flash"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash"));
}

/* ── vfs_list_dir ─────────────────────────────────────────────────────── */

TEST_CASE("list_dir returns created entries", "[vfs]")
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/list_a.txt", "a", 1));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/list_b.txt", "bb", 2));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/list_sub"));

    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(vfs_dir_entry_t));
    TEST_ASSERT_NOT_NULL(entries);

    size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_list_dir("/flash", entries, VFS_ENTRIES_MAX, &count));
    TEST_ASSERT_GREATER_OR_EQUAL(3, count);

    bool found_a = false, found_b = false, found_sub = false;
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(entries[i].name, "list_a.txt") == 0)
        {
            found_a = true;
            TEST_ASSERT_FALSE(entries[i].is_dir);
        }
        if (strcmp(entries[i].name, "list_b.txt") == 0)
        {
            found_b = true;
            TEST_ASSERT_FALSE(entries[i].is_dir);
        }
        if (strcmp(entries[i].name, "list_sub") == 0)
        {
            found_sub = true;
            TEST_ASSERT_TRUE(entries[i].is_dir);
        }
    }
    free(entries);
    TEST_ASSERT_TRUE(found_a);
    TEST_ASSERT_TRUE(found_b);
    TEST_ASSERT_TRUE(found_sub);
}

TEST_CASE("list_dir virtual root shows flash", "[vfs]")
{
    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(vfs_dir_entry_t));
    TEST_ASSERT_NOT_NULL(entries);

    size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, vfs_list_dir("/", entries, VFS_ENTRIES_MAX, &count));
    TEST_ASSERT_GREATER_OR_EQUAL(1, count);
    TEST_ASSERT_EQUAL_STRING("flash", entries[0].name);
    TEST_ASSERT_TRUE(entries[0].is_dir);
    free(entries);
}

/* ── vfs_mkdir ────────────────────────────────────────────────────────── */

TEST_CASE("mkdir creates nested directories", "[vfs]")
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/nest1"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/nest1/nest2"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash/nest1/nest2"));
}

/* ── vfs_remove ───────────────────────────────────────────────────────── */

TEST_CASE("remove deletes file", "[vfs]")
{
    const char *path = "/flash/removeme.txt";
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file(path, "bye", 3));
    TEST_ASSERT_TRUE(vfs_exists(path));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_remove(path));
    TEST_ASSERT_FALSE(vfs_exists(path));
}

TEST_CASE("remove empty directory", "[vfs]")
{
    TEST_ASSERT_EQUAL(ESP_OK, vfs_mkdir("/flash/rmdir_test"));
    TEST_ASSERT_TRUE(vfs_is_directory("/flash/rmdir_test"));
    TEST_ASSERT_EQUAL(ESP_OK, vfs_remove("/flash/rmdir_test"));
    TEST_ASSERT_FALSE(vfs_exists("/flash/rmdir_test"));
}

/* ── Flash usage ──────────────────────────────────────────────────────── */

TEST_CASE("flash total bytes is nonzero", "[vfs]")
{
    TEST_ASSERT_GREATER_THAN(0, flash_get_total_bytes());
}

TEST_CASE("flash used bytes grows after write", "[vfs]")
{
    size_t before = flash_get_used_bytes();
    char *big = malloc(4096);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'X', 4096);
    TEST_ASSERT_EQUAL(ESP_OK, vfs_write_file("/flash/grow.bin", big, 4096));
    free(big);
    size_t after = flash_get_used_bytes();
    TEST_ASSERT_GREATER_THAN(before, after);
}

/* ── Edge cases ───────────────────────────────────────────────────────── */

TEST_CASE("read non-existent file returns error", "[vfs]")
{
    char buf[32];
    size_t bytes_read = 0;
    TEST_ASSERT_NOT_EQUAL(ESP_OK, vfs_read_file("/flash/ghost.dat", buf, sizeof(buf), &bytes_read));
}

TEST_CASE("remove non-existent file returns error", "[vfs]")
{
    TEST_ASSERT_NOT_EQUAL(ESP_OK, vfs_remove("/flash/no_file.dat"));
}

TEST_CASE("null arguments rejected", "[vfs]")
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_write_file(NULL, "x", 1));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_read_file(NULL, NULL, 0, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_list_dir(NULL, NULL, 0, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_mkdir(NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, vfs_remove(NULL));
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(filesystem_init());
    ESP_ERROR_CHECK(flash_format());

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}
