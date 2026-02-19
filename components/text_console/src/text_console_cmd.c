#include "text_console.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "text_console_cmd";

static int cmd_display_mode(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("Display mode: text\n");
        return 0;
    }

    if (strcmp(argv[1], "text") == 0)
    {
        printf("Display mode: text (already active)\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0)
    {
        text_console_clear();
        return 0;
    }

    printf("Usage: display_mode [text|clear]\n");
    return 1;
}

void text_console_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "display_mode",
        .help = "Display mode control",
        .hint = "[text|clear]",
        .func = &cmd_display_mode,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'display_mode' command");
}
