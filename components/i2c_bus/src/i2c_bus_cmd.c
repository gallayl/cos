#include "i2c_bus.h"

#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "i2c_cmd";

static int cmd_i2c(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: i2c scan | read <addr> <size> | write <addr> <bytes...>\n");
        return 1;
    }

    const char *subcmd = argv[1];

    /* i2c scan */
    if (strcmp(subcmd, "scan") == 0)
    {
        uint8_t addrs[I2C_BUS_MAX_DEVICES];
        size_t found = 0;
        esp_err_t err = i2c_bus_scan(addrs, I2C_BUS_MAX_DEVICES, &found);
        if (err != ESP_OK)
        {
            printf("i2c: scan failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Found %u device(s):\n", (unsigned)found);
        for (size_t i = 0; i < found; i++)
        {
            printf("  0x%02X\n", addrs[i]);
        }
        return 0;
    }

    /* i2c read <addr> <size> */
    if (strcmp(subcmd, "read") == 0)
    {
        if (argc < 4)
        {
            printf("Usage: i2c read <hex_addr> <size>\n");
            return 1;
        }
        uint8_t addr = (uint8_t)strtol(argv[2], NULL, 0);
        int size = atoi(argv[3]);
        if (size <= 0 || size > 256)
        {
            printf("i2c: size must be 1-256\n");
            return 1;
        }
        uint8_t buf[256];
        esp_err_t err = i2c_bus_read(addr, buf, (size_t)size);
        if (err != ESP_OK)
        {
            printf("i2c: read failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        for (int i = 0; i < size; i++)
        {
            printf("%02X%s", buf[i], (i < size - 1) ? " " : "\n");
        }
        return 0;
    }

    /* i2c write <addr> <bytes...> */
    if (strcmp(subcmd, "write") == 0)
    {
        if (argc < 4)
        {
            printf("Usage: i2c write <hex_addr> <hex_byte> [<hex_byte> ...]\n");
            return 1;
        }
        uint8_t addr = (uint8_t)strtol(argv[2], NULL, 0);
        int nbytes = argc - 3;
        if (nbytes > 256)
        {
            printf("i2c: too many bytes (max 256)\n");
            return 1;
        }
        uint8_t data[256];
        for (int i = 0; i < nbytes; i++)
        {
            data[i] = (uint8_t)strtol(argv[3 + i], NULL, 0);
        }
        esp_err_t err = i2c_bus_write(addr, data, (size_t)nbytes);
        if (err != ESP_OK)
        {
            printf("i2c: write failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Wrote %d byte(s) to 0x%02X\n", nbytes, addr);
        return 0;
    }

    printf("i2c: unknown subcommand '%s'\n", subcmd);
    printf("Usage: i2c scan | read <addr> <size> | write <addr> <bytes...>\n");
    return 1;
}

void i2c_bus_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "i2c",
        .help = "I2C bus operations (scan, read, write)",
        .hint = "<scan|read|write>",
        .func = &cmd_i2c,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'i2c' command");
}
