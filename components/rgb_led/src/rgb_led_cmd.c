#include "rgb_led.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "rgb_led_cmd";

static int cmd_led(int argc, char **argv)
{
    if (argc == 1)
    {
        uint8_t r, g, b;
        rgb_led_get_color(&r, &g, &b);
        printf("LED color: %u %u %u\n", r, g, b);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "off") == 0)
    {
        esp_err_t err = rgb_led_off();
        if (err != ESP_OK)
        {
            printf("led: failed to turn off (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("LED off\n");
        return 0;
    }

    if (argc == 4)
    {
        int r = atoi(argv[1]);
        int g = atoi(argv[2]);
        int b = atoi(argv[3]);

        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
        {
            printf("led: values must be 0-255\n");
            return 1;
        }

        esp_err_t err = rgb_led_set_color((uint8_t)r, (uint8_t)g, (uint8_t)b);
        if (err != ESP_OK)
        {
            printf("led: failed to set color (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("LED color: %d %d %d\n", r, g, b);
        return 0;
    }

    printf("Usage: led [<r> <g> <b> | off]\n");
    return 1;
}

void rgb_led_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "led",
        .help = "Get/set RGB LED color (0-255 each) or turn off",
        .hint = "[<r> <g> <b> | off]",
        .func = &cmd_led,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'led' command");
}
