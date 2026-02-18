#include "filesystem.h"
#include "system.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>

static const char *const TAG = "system_cmd";

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    system_info_t info;
    esp_err_t err = system_get_info(&info);
    if (err != ESP_OK)
    {
        printf("info: failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    printf("IDF version: %s\n", info.idf_version);
    printf("CPU freq:    %lu MHz\n", (unsigned long)info.cpu_freq_mhz);
    printf("Flash size:  %lu bytes\n", (unsigned long)info.flash_size);
    printf("Free heap:   %u bytes\n", (unsigned)info.free_heap);

    printf("Flash FS:    %u total, %u used\n", (unsigned)flash_get_total_bytes(), (unsigned)flash_get_used_bytes());

    if (sdcard_is_mounted())
    {
        printf("SD card:     %s, %llu total, %llu used\n", sdcard_get_type_name(),
               (unsigned long long)sdcard_get_total_bytes(), (unsigned long long)sdcard_get_used_bytes());
    }
    else
    {
        printf("SD card:     not mounted\n");
    }
    return 0;
}

static int cmd_memory(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    system_info_t info;
    esp_err_t err = system_get_info(&info);
    if (err != ESP_OK)
    {
        printf("memory: failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    printf("Free heap:      %u bytes\n", (unsigned)info.free_heap);
    printf("Min free heap:  %u bytes\n", (unsigned)info.min_free_heap);
    printf("Total heap:     %u bytes\n", (unsigned)info.total_heap);
    printf("Max alloc:      %u bytes\n", (unsigned)info.max_alloc_block);
    return 0;
}

static int cmd_restart(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Restarting...\n");
    system_restart();
    return 0;
}

static int cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    char buf[32];
    esp_err_t err = system_format_uptime(buf, sizeof(buf));
    if (err != ESP_OK)
    {
        printf("uptime: error\n");
        return 1;
    }
    printf("Up %s\n", buf);
    return 0;
}

void system_register_commands(void)
{
    const esp_console_cmd_t info_cmd = {
        .command = "info",
        .help = "Show system and filesystem information",
        .hint = NULL,
        .func = &cmd_info,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&info_cmd));

    const esp_console_cmd_t memory_cmd = {
        .command = "memory",
        .help = "Show detailed heap memory statistics",
        .hint = NULL,
        .func = &cmd_memory,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&memory_cmd));

    const esp_console_cmd_t restart_cmd = {
        .command = "restart",
        .help = "Restart the device",
        .hint = NULL,
        .func = &cmd_restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&restart_cmd));

    const esp_console_cmd_t uptime_cmd = {
        .command = "uptime",
        .help = "Show time since boot",
        .hint = NULL,
        .func = &cmd_uptime,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&uptime_cmd));

    ESP_LOGI(TAG, "System commands registered");
}
