#include "time_sync.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "time_cmd";

static int cmd_time(int argc, char **argv)
{
    /* time epoch */
    if (argc >= 2 && strcmp(argv[1], "epoch") == 0)
    {
        printf("%lld\n", (long long)time_sync_get_epoch());
        return 0;
    }

    /* time utc */
    if (argc >= 2 && strcmp(argv[1], "utc") == 0)
    {
        char buf[32];
        esp_err_t err = time_sync_get_utc(buf, sizeof(buf));
        if (err != ESP_OK)
        {
            printf("time: format error (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("%s\n", buf);
        return 0;
    }

    /* time (no args) -- full status */
    char local_buf[32];
    char utc_buf[32];
    esp_err_t err = time_sync_get_local(local_buf, sizeof(local_buf));
    if (err != ESP_OK)
    {
        printf("time: format error (%s)\n", esp_err_to_name(err));
        return 1;
    }
    err = time_sync_get_utc(utc_buf, sizeof(utc_buf));
    if (err != ESP_OK)
    {
        printf("time: format error (%s)\n", esp_err_to_name(err));
        return 1;
    }
    printf("Local:  %s\n", local_buf);
    printf("UTC:    %s\n", utc_buf);
    printf("Synced: %s\n", time_sync_is_synced() ? "yes" : "no");
    return 0;
}

void time_sync_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "time",
        .help = "Show current time and sync status",
        .hint = "[utc|epoch]",
        .func = &cmd_time,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'time' command");
}
