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

static int cmd_logging(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("Logging level: %s\n", text_console_log_level_name(text_console_get_log_level()));
        return 0;
    }

    if (argc != 2)
    {
        printf("Usage: logging [error|warn|info]\n");
        return 1;
    }

    text_console_log_level_t level;
    if (strcmp(argv[1], "error") == 0)
    {
        level = TEXT_CONSOLE_LOG_ERROR;
    }
    else if (strcmp(argv[1], "warn") == 0)
    {
        level = TEXT_CONSOLE_LOG_WARN;
    }
    else if (strcmp(argv[1], "info") == 0)
    {
        level = TEXT_CONSOLE_LOG_INFO;
    }
    else
    {
        printf("Usage: logging [error|warn|info]\n");
        return 1;
    }

    esp_err_t err = text_console_set_log_level(level);
    if (err != ESP_OK)
    {
        printf("logging: could not save level (%s)\n", esp_err_to_name(err));
        return 1;
    }
    printf("Logging level set to %s (global: UART and console)\n", text_console_log_level_name(level));
    return 0;
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

    const esp_console_cmd_t logging_cmd = {
        .command = "logging",
        .help = "Set the persistent global ESP log threshold",
        .hint = "[error|warn|info]",
        .func = &cmd_logging,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&logging_cmd));
    ESP_LOGI(TAG, "Registered 'logging' command");
}
