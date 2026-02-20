#include "unity.h"

#include "filesystem.h"
#include "shell.h"
#include "system.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "test_stability";

#define HEAP_LEAK_TOLERANCE_BYTES 2048
#define MIN_FREE_HEAP_THRESHOLD 8192
#define MIN_STACK_WATERMARK_WORDS 64

static void run_fs_workload(void)
{
    for (int i = 0; i < 10; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/flash/wl_%d.txt", i);
        vfs_write_file(path, "workload data payload", 21);
    }

    for (int i = 0; i < 10; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/flash/wl_%d.txt", i);

        char buf[64];
        size_t bytes_read = 0;
        vfs_read_file(path, buf, sizeof(buf), &bytes_read);
    }

    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(vfs_dir_entry_t));
    if (entries != NULL)
    {
        size_t count = 0;
        vfs_list_dir("/flash", entries, VFS_ENTRIES_MAX, &count);
        free(entries);
    }

    for (int i = 0; i < 10; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/flash/wl_%d.txt", i);
        vfs_remove(path);
    }

    vfs_mkdir("/flash/wl_dir");
    vfs_remove("/flash/wl_dir");
}

/* ── Heap leak detection ──────────────────────────────────────────────── */

TEST_CASE("heap stable after filesystem workload", "[stability]")
{
    size_t before = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap before workload: %u", (unsigned)before);

    for (int round = 0; round < 5; round++)
    {
        run_fs_workload();
    }

    size_t after = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap after workload:  %u (delta=%d)", (unsigned)after, (int)before - (int)after);

    int leak = (int)before - (int)after;
    TEST_ASSERT_MESSAGE(leak < HEAP_LEAK_TOLERANCE_BYTES,
                        "Heap leak detected: free heap dropped beyond tolerance");
}

/* ── Minimum heap watermark ───────────────────────────────────────────── */

TEST_CASE("min free heap above threshold", "[stability]")
{
    run_fs_workload();

    size_t min_heap = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG, "Minimum free heap since boot: %u", (unsigned)min_heap);
    TEST_ASSERT_GREATER_THAN_MESSAGE(MIN_FREE_HEAP_THRESHOLD, min_heap,
                                     "Minimum free heap dropped below safety threshold");
}

/* ── Task stack watermarks ────────────────────────────────────────────── */

TEST_CASE("task stack watermarks above margin", "[stability]")
{
    run_fs_workload();

    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_array = malloc(task_count * sizeof(TaskStatus_t));
    TEST_ASSERT_NOT_NULL(task_array);

    UBaseType_t actual = uxTaskGetSystemState(task_array, task_count, NULL);
    ESP_LOGI(TAG, "Checking %u tasks for stack watermarks", (unsigned)actual);

    bool all_ok = true;
    for (UBaseType_t i = 0; i < actual; i++)
    {
        ESP_LOGI(TAG, "  %-16s stack_hwm=%u words", task_array[i].pcTaskName,
                 (unsigned)task_array[i].usStackHighWaterMark);

        if (task_array[i].usStackHighWaterMark < MIN_STACK_WATERMARK_WORDS)
        {
            ESP_LOGE(TAG, "  FAIL: %s has only %u words of stack remaining", task_array[i].pcTaskName,
                     (unsigned)task_array[i].usStackHighWaterMark);
            all_ok = false;
        }
    }

    free(task_array);
    TEST_ASSERT_MESSAGE(all_ok, "One or more tasks have dangerously low stack watermarks");
}

/* ── Repeated format cycle (stress) ───────────────────────────────────── */

TEST_CASE("repeated format does not leak", "[stability]")
{
    size_t before = esp_get_free_heap_size();

    for (int i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL(ESP_OK, flash_format());
        vfs_write_file("/flash/fmt_test.txt", "after format", 12);
        vfs_remove("/flash/fmt_test.txt");
    }

    size_t after = esp_get_free_heap_size();
    int leak = (int)before - (int)after;
    ESP_LOGI(TAG, "Format cycle heap delta: %d bytes", leak);
    TEST_ASSERT_MESSAGE(leak < HEAP_LEAK_TOLERANCE_BYTES,
                        "Heap leak detected after repeated format cycles");
}

/* ── Entry point ──────────────────────────────────────────────────────── */

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

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(filesystem_init());
    ESP_ERROR_CHECK(flash_format());
    ESP_ERROR_CHECK(system_init());
    ESP_ERROR_CHECK(shell_init());

    ESP_LOGI(TAG, "Stability test starting");

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}
