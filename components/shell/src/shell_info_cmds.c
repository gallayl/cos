#include "shell.h"
#include "filesystem.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>

static const char *const TAG = "shell_info";

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Flash (LittleFS):\n");
    printf("  Total: %u bytes\n", (unsigned)flash_get_total_bytes());
    printf("  Used:  %u bytes\n", (unsigned)flash_get_used_bytes());

    if (sdcard_is_mounted())
    {
        printf("SD card (%s):\n", sdcard_get_type_name());
        printf("  Total: %llu bytes\n", (unsigned long long)sdcard_get_total_bytes());
        printf("  Used:  %llu bytes\n", (unsigned long long)sdcard_get_used_bytes());
    }
    else
    {
        printf("SD card: not mounted\n");
    }

    return 0;
}

static int cmd_format(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Formatting flash filesystem...\n");
    esp_err_t err = flash_format();
    if (err != ESP_OK)
    {
        printf("format: failed (%s)\n", esp_err_to_name(err));
        return 1;
    }
    printf("Format complete\n");
    return 0;
}

void shell_register_info_commands(void)
{
    const esp_console_cmd_t info_cmd = {
        .command = "info",
        .help = "Show filesystem usage statistics",
        .hint = NULL,
        .func = &cmd_info,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&info_cmd));

    const esp_console_cmd_t format_cmd = {
        .command = "format",
        .help = "Format flash (LittleFS) filesystem",
        .hint = NULL,
        .func = &cmd_format,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&format_cmd));

    ESP_LOGI(TAG, "Info commands registered");
}
