#include "filesystem.h"
#include "shell.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>

static const char *const TAG = "shell_info";

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
    const esp_console_cmd_t format_cmd = {
        .command = "format",
        .help = "Format flash (LittleFS) filesystem",
        .hint = NULL,
        .func = &cmd_format,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&format_cmd));

    ESP_LOGI(TAG, "Filesystem commands registered (info provided by Berry script)");
}
