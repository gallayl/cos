#include "light_sensor.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>

static const char *const TAG = "light_cmd";

static int cmd_light(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int value = light_sensor_read();
    if (value < 0)
    {
        printf("light: read failed\n");
        return 1;
    }
    printf("Light sensor: %d\n", value);
    return 0;
}

void light_sensor_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "light",
        .help = "Read ambient light sensor value (0-4095)",
        .hint = NULL,
        .func = &cmd_light,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'light' command");
}
