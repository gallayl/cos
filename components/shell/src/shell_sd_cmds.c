#include "filesystem.h"
#include "shell.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "shell_sd";

static int cmd_sd(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: sd mount | unmount | info\n");
        return 1;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "mount") == 0)
    {
        esp_err_t err = sdcard_mount();
        if (err != ESP_OK)
        {
            printf("sd: mount failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("SD card mounted (%s)\n", sdcard_get_type_name());
        return 0;
    }

    if (strcmp(subcmd, "unmount") == 0)
    {
        esp_err_t err = sdcard_unmount();
        if (err != ESP_OK)
        {
            printf("sd: unmount failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("SD card unmounted\n");
        return 0;
    }

    if (strcmp(subcmd, "info") == 0)
    {
        if (!sdcard_is_mounted())
        {
            printf("SD card not mounted\n");
            return 1;
        }
        printf("Type:  %s\n", sdcard_get_type_name());
        printf("Total: %llu bytes\n", (unsigned long long)sdcard_get_total_bytes());
        printf("Used:  %llu bytes\n", (unsigned long long)sdcard_get_used_bytes());
        return 0;
    }

    printf("sd: unknown subcommand '%s'\n", subcmd);
    printf("Usage: sd mount | unmount | info\n");
    return 1;
}

void shell_register_sd_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "sd",
        .help = "SD card management (mount, unmount, info)",
        .hint = "<mount|unmount|info>",
        .func = &cmd_sd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "SD card commands registered");
}
