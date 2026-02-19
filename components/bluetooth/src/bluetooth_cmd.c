#include "bluetooth.h"

#include "esp_bt_defs.h"
#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "bt_cmd";

static bool parse_bda(const char *str, esp_bd_addr_t bda)
{
    unsigned int v[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6)
    {
        return false;
    }
    for (int i = 0; i < 6; i++)
    {
        if (v[i] > 0xFF)
        {
            return false;
        }
        bda[i] = (uint8_t)v[i];
    }
    return true;
}

static void print_status(void)
{
    if (bluetooth_is_enabling())
    {
        printf("Bluetooth: enabling...\n");
    }
    else
    {
        printf("Bluetooth: %s\n", bluetooth_is_enabled() ? "ON" : "OFF");
    }

    if (bluetooth_is_connected())
    {
        const char *name = bluetooth_connected_device_name();
        const uint8_t *bda = bluetooth_connected_device_bda();
        if (bda != NULL)
        {
            printf("Connected: %s (" ESP_BD_ADDR_STR ")\n", name ? name : "(unknown)", ESP_BD_ADDR_HEX(bda));
        }
    }
    else if (bluetooth_is_enabled())
    {
        printf("Connected: none\n");
    }
}

static int cmd_bt(int argc, char **argv)
{
    if (argc < 2)
    {
        print_status();
        return 0;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "on") == 0)
    {
        esp_err_t err = bluetooth_enable();
        if (err != ESP_OK)
        {
            printf("bt: enable failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Bluetooth enabling in background...\n");
        return 0;
    }

    if (strcmp(subcmd, "off") == 0)
    {
        esp_err_t err = bluetooth_disable();
        if (err != ESP_OK)
        {
            printf("bt: disable failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Bluetooth disabled\n");
        return 0;
    }

    if (strcmp(subcmd, "scan") == 0)
    {
        if (!bluetooth_is_enabled())
        {
            printf("bt: not enabled (run 'bt on' first)\n");
            return 1;
        }
        esp_err_t err = bluetooth_scan();
        if (err != ESP_OK)
        {
            printf("bt: scan failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        return 0;
    }

    if (strcmp(subcmd, "connect") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: bt connect <aa:bb:cc:dd:ee:ff>\n");
            return 1;
        }
        if (!bluetooth_is_enabled())
        {
            printf("bt: not enabled (run 'bt on' first)\n");
            return 1;
        }
        if (!bluetooth_is_hid_ready())
        {
            printf("bt: HID host not ready yet (wait a few seconds after 'bt on')\n");
            return 1;
        }
        esp_bd_addr_t bda;
        if (!parse_bda(argv[2], bda))
        {
            printf("bt: invalid address '%s'\n", argv[2]);
            return 1;
        }
        esp_err_t err = bluetooth_connect(bda);
        if (err != ESP_OK)
        {
            printf("bt: connect failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Connecting to " ESP_BD_ADDR_STR "...\n", ESP_BD_ADDR_HEX(bda));
        return 0;
    }

    if (strcmp(subcmd, "disconnect") == 0)
    {
        esp_err_t err = bluetooth_disconnect();
        if (err != ESP_OK)
        {
            printf("bt: disconnect failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Disconnecting...\n");
        return 0;
    }

    printf("Usage: bt [on | off | scan | connect <addr> | disconnect]\n");
    return 1;
}

void bluetooth_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "bt",
        .help = "Bluetooth management (on, off, scan, connect, disconnect)",
        .hint = "[on | off | scan | connect <addr> | disconnect]",
        .func = &cmd_bt,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'bt' command");
}
