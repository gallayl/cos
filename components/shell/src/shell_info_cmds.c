#include "filesystem.h"
#include "service_diagnostics.h"
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

static int cmd_svc_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    esp_err_t err = service_diag_print_svc_mem();
    return err == ESP_OK ? 0 : 1;
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

    const esp_console_cmd_t svc_mem_cmd = {
        .command = "svc_mem",
        .help = "Show on-demand service memory/diagnostics snapshot",
        .hint = NULL,
        .func = &cmd_svc_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&svc_mem_cmd));

    ESP_LOGI(TAG, "Filesystem/diagnostics commands registered (info provided by Berry script)");
}
