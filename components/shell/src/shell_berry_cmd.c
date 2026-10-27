#include "berry_runtime.h"
#include "filesystem.h"
#include "shell.h"

#include "esp_console.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_berry(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: berry run <path> | berry eval <source> | berry close\n");
        return 1;
    }

    esp_err_t err = berry_runtime_init();
    if (err != ESP_OK)
    {
        printf("berry: initialization failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    if (strcmp(argv[1], "close") == 0)
    {
        if (argc != 2)
        {
            printf("Usage: berry close\n");
            return 1;
        }
        err = berry_runtime_close_app();
    }
    else if (strcmp(argv[1], "run") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: berry run <path>\n");
            return 1;
        }
        char path[VFS_PATH_MAX];
        err = shell_resolve_relative(argv[2], path, sizeof(path));
        if (err == ESP_OK)
        {
            err = berry_runtime_run_file(path);
        }
    }
    else if (strcmp(argv[1], "eval") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: berry eval <source>\n");
            return 1;
        }
        size_t source_size = 1;
        for (int i = 2; i < argc; ++i)
        {
            size_t arg_size = strlen(argv[i]);
            size_t separator_size = i > 2 ? 1 : 0;
            if (source_size > SIZE_MAX - separator_size || arg_size > SIZE_MAX - source_size - separator_size)
            {
                printf("berry: source is too large\n");
                return 1;
            }
            source_size += arg_size + separator_size;
        }

        char *source = (char *)malloc(source_size);
        if (source == NULL)
        {
            printf("berry: out of memory\n");
            return 1;
        }

        size_t offset = 0;
        for (int i = 2; i < argc; ++i)
        {
            if (i > 2)
            {
                source[offset++] = ' ';
            }
            size_t arg_size = strlen(argv[i]);
            memcpy(source + offset, argv[i], arg_size);
            offset += arg_size;
        }
        source[offset] = '\0';
        err = berry_runtime_eval(source, "shell");
        free(source);
    }
    else
    {
        printf("berry: unknown operation '%s'\n", argv[1]);
        printf("Usage: berry run <path> | berry eval <source> | berry close\n");
        return 1;
    }

    if (err != ESP_OK)
    {
        printf("berry: execution failed (%s)\n", esp_err_to_name(err));
        return 1;
    }

    return 0;
}

void shell_register_berry_commands(void)
{
    const esp_console_cmd_t command = {
        .command = "berry",
        .help = "Run a Berry script or evaluate Berry source",
        .hint = NULL,
        .func = &cmd_berry,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}
