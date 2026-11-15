#include "network.h"
#include "shell_cmds.h"

#include "esp_console.h"
#include "esp_log.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static const char *const TAG = "shell_network";
#define DEFAULT_NETWORK_TIMEOUT_MS 1000

static esp_err_t parse_timeout(int argc, char **argv, uint32_t *timeout_ms)
{
    *timeout_ms = DEFAULT_NETWORK_TIMEOUT_MS;
    if (argc == 2)
    {
        return ESP_OK;
    }

    errno = 0;
    char *end = NULL;
    const unsigned long parsed = strtoul(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || parsed < NETWORK_PING_TIMEOUT_MIN_MS ||
        parsed > NETWORK_PING_TIMEOUT_MAX_MS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *timeout_ms = (uint32_t)parsed;
    return ESP_OK;
}

static int cmd_nslookup(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        printf("Usage: nslookup <host> [timeout_ms]\n");
        return 1;
    }

    uint32_t timeout_ms;
    esp_err_t err = parse_timeout(argc, argv, &timeout_ms);
    if (err != ESP_OK)
    {
        printf("nslookup: timeout must be %u-%u ms\n", NETWORK_PING_TIMEOUT_MIN_MS, NETWORK_PING_TIMEOUT_MAX_MS);
        return 1;
    }

    network_ping_result_t result = {0};
    err = network_resolve(argv[1], timeout_ms, &result);
    if (err != ESP_OK)
    {
        printf("nslookup: %s failed (%s)\n", argv[1], esp_err_to_name(err));
        return 1;
    }

    printf("Name: %s\nAddress: %s\n", argv[1], result.address);
    return 0;
}

void shell_register_network_commands(void)
{
    const esp_console_cmd_t nslookup_cmd = {
        .command = "nslookup",
        .help = "Resolve a hostname to IPv4",
        .hint = "<host> [timeout_ms]",
        .func = &cmd_nslookup,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&nslookup_cmd));

    ESP_LOGI(TAG, "Registered nslookup command (ping provided by Berry script)");
}
