#include "brightness.h"
#include "calibration.h"
#include "display.h"
#include "text_console.h"

#include "esp_console.h"
#include "esp_log.h"
#include "nvs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "brightness_cmd";

#define BRIGHTNESS_NVS_NAMESPACE "brightness"
#define BRIGHTNESS_NVS_KEY_ROTATION "rotation"

static esp_err_t save_rotation(uint8_t rotation)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(BRIGHTNESS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }
    err = nvs_set_blob(handle, BRIGHTNESS_NVS_KEY_ROTATION, &rotation, sizeof(rotation));
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void print_status(void)
{
    printf("Brightness: %u (%s)\n", brightness_get(), brightness_is_auto() ? "auto" : "manual");
    printf("Rotation:   %u\n", display_get_rotation());
}

static int cmd_screen(int argc, char **argv)
{
    if (argc == 1)
    {
        print_status();
        return 0;
    }

    /* screen brightness ... */
    if (strcmp(argv[1], "brightness") == 0)
    {
        if (argc == 2)
        {
            printf("Brightness: %u (%s)\n", brightness_get(), brightness_is_auto() ? "auto" : "manual");
            return 0;
        }

        if (strcmp(argv[2], "auto") == 0)
        {
            esp_err_t err = brightness_set_auto(true);
            if (err != ESP_OK)
            {
                printf("screen: failed to enable auto mode (%s)\n", esp_err_to_name(err));
                return 1;
            }
            printf("Brightness: auto\n");
            return 0;
        }

        int val = atoi(argv[2]);
        if (val < 0 || val > 255)
        {
            printf("screen: brightness must be 0-255\n");
            return 1;
        }
        esp_err_t err = brightness_set((uint8_t)val);
        if (err != ESP_OK)
        {
            printf("screen: failed to set brightness (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Brightness: %d (manual)\n", val);
        return 0;
    }

    /* screen rotation ... */
    if (strcmp(argv[1], "rotation") == 0)
    {
        if (argc == 2)
        {
            printf("Rotation: %u\n", display_get_rotation());
            return 0;
        }

        int rot = atoi(argv[2]);
        if (rot < 0 || rot > 7)
        {
            printf("screen: rotation must be 0-7\n");
            return 1;
        }

        esp_err_t err = display_set_rotation((uint8_t)rot);
        if (err != ESP_OK)
        {
            printf("screen: failed to set rotation (%s)\n", esp_err_to_name(err));
            return 1;
        }

        save_rotation((uint8_t)rot);
        calibration_load();
        text_console_resize();

        printf("Rotation: %d\n", rot);
        return 0;
    }

    printf("Usage: screen [brightness [<0-255>|auto] | rotation [<0-7>]]\n");
    return 1;
}

void brightness_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "screen",
        .help = "Screen brightness and rotation settings",
        .hint = "[brightness [<0-255>|auto] | rotation [<0-7>]]",
        .func = &cmd_screen,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'screen' command");
}
