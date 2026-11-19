#include "http_server.h"

#include "boot_profile.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "http_cmd";

static void print_status(void)
{
    printf("HTTP server: %s\n", http_server_is_running() ? "running" : "stopped");
    printf("Static:      %s\n", http_static_is_enabled() ? "on" : "off");
    printf("Static root: %s\n", http_static_get_root());
    printf("SPA:         %s\n", http_static_is_spa() ? "on" : "off");
    printf("Auth user:   %s\n", http_auth_get_username());
}

static int cmd_server(int argc, char **argv)
{
    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "status") == 0))
    {
        print_status();
        return 0;
    }

    /* server start */
    if (strcmp(argv[1], "start") == 0)
    {
        if (!boot_profile_allows_http_server())
        {
            printf("server: start failed: forced off by boot profile for this boot\n");
            return 1;
        }
        if (http_server_is_running())
        {
            printf("HTTP server already running\n");
            return 0;
        }
        esp_err_t err = http_server_start();
        if (err != ESP_OK)
        {
            printf("server: start failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("HTTP server started\n");
        return 0;
    }

    /* server stop */
    if (strcmp(argv[1], "stop") == 0)
    {
        if (!http_server_is_running())
        {
            printf("HTTP server already stopped\n");
            return 0;
        }
        /* Dependency-safe: pre-stop hook stops WebSocket if running. */
        esp_err_t err = http_server_stop();
        if (err != ESP_OK)
        {
            printf("server: stop failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("HTTP server stopped\n");
        return 0;
    }

    /* server static ... */
    if (strcmp(argv[1], "static") == 0)
    {
        if (argc == 2)
        {
            printf("Static:      %s\n", http_static_is_enabled() ? "on" : "off");
            printf("Static root: %s\n", http_static_get_root());
            printf("SPA:         %s\n", http_static_is_spa() ? "on" : "off");
            return 0;
        }

        if (strcmp(argv[2], "on") == 0)
        {
            http_static_set_enabled(true);
            printf("Static file serving: on\n");
            return 0;
        }
        if (strcmp(argv[2], "off") == 0)
        {
            http_static_set_enabled(false);
            printf("Static file serving: off\n");
            return 0;
        }

        if (strcmp(argv[2], "root") == 0)
        {
            if (argc < 4)
            {
                printf("Static root: %s\n", http_static_get_root());
                return 0;
            }
            esp_err_t err = http_static_set_root(argv[3]);
            if (err != ESP_OK)
            {
                printf("server: invalid root path\n");
                return 1;
            }
            printf("Static root: %s\n", http_static_get_root());
            return 0;
        }

        if (strcmp(argv[2], "spa") == 0)
        {
            if (argc < 4)
            {
                printf("SPA: %s\n", http_static_is_spa() ? "on" : "off");
                return 0;
            }
            if (strcmp(argv[3], "on") == 0)
            {
                http_static_set_spa(true);
                printf("SPA fallback: on\n");
                return 0;
            }
            if (strcmp(argv[3], "off") == 0)
            {
                http_static_set_spa(false);
                printf("SPA fallback: off\n");
                return 0;
            }
            printf("Usage: server static spa on|off\n");
            return 1;
        }

        printf("Usage: server static on|off|root <path>|spa on|off\n");
        return 1;
    }

    /* server auth ... */
    if (strcmp(argv[1], "auth") == 0)
    {
        if (argc == 2)
        {
            printf("Auth user: %s\n", http_auth_get_username());
            return 0;
        }
        if (argc < 4)
        {
            printf("Usage: server auth <username> <password>\n");
            return 1;
        }
        esp_err_t err = http_auth_set_credentials(argv[2], argv[3]);
        if (err != ESP_OK)
        {
            printf("server: invalid credentials\n");
            return 1;
        }
        printf("Auth updated (user=%s)\n", http_auth_get_username());
        return 0;
    }

    printf("Usage: server [status|start|stop|static ...|auth ...]\n");
    return 1;
}

void http_server_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "server",
        .help = "HTTP server management (status, start, stop, static, auth)",
        .hint = "[status|start|stop|static|auth]",
        .func = &cmd_server,
    };
    esp_err_t err = esp_console_cmd_register(&cmd);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Registered 'server' command");
    }
    else if (err == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGD(TAG, "'server' command already registered");
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }
}
