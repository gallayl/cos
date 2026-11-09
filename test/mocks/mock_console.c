#include "mock_console.h"
#include "esp_console.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MOCK_MAX_COMMANDS 64
#define MOCK_MAX_ARGS 16

static struct
{
    esp_console_cmd_t command;
} s_commands[MOCK_MAX_COMMANDS];

static int s_cmd_count = 0;

static bool command_registered(const char *name)
{
    for (int i = 0; i < s_cmd_count; i++)
    {
        if (strcmp(s_commands[i].command.command, name) == 0)
        {
            return true;
        }
    }
    return false;
}

void mock_console_reset(void)
{
    s_cmd_count = 0;
}

esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd)
{
    if (cmd == NULL || s_cmd_count >= MOCK_MAX_COMMANDS)
    {
        return ESP_FAIL;
    }
    if (command_registered(cmd->command))
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_commands[s_cmd_count].command = *cmd;
    s_cmd_count++;
    return ESP_OK;
}

esp_err_t esp_console_register_help_command(void)
{
    return ESP_OK;
}

void linenoiseAddCompletion(linenoiseCompletions *completions, const char *text)
{
    char *copy = malloc(strlen(text) + 1);
    if (copy == NULL)
    {
        return;
    }
    strcpy(copy, text);

    char **new_cvec = realloc(completions->cvec, (completions->len + 1) * sizeof(*new_cvec));
    if (new_cvec == NULL)
    {
        free(copy);
        return;
    }
    completions->cvec = new_cvec;
    completions->cvec[completions->len++] = copy;
}

void esp_console_get_completion(const char *buf, linenoiseCompletions *completions)
{
    size_t prefix_len = strlen(buf);
    for (int i = 0; i < s_cmd_count; i++)
    {
        if (strncmp(buf, s_commands[i].command.command, prefix_len) == 0)
        {
            linenoiseAddCompletion(completions, s_commands[i].command.command);
        }
    }
}

int mock_console_run_cmd(const char *name, int argc, char **argv)
{
    for (int i = 0; i < s_cmd_count; i++)
    {
        if (strcmp(s_commands[i].command.command, name) == 0)
        {
            return s_commands[i].command.func(argc, argv);
        }
    }
    return -1;
}

size_t esp_console_split_argv(char *line, char **argv, size_t argv_size)
{
    size_t argc = 0;
    char *p = line;

    while (*p != '\0')
    {
        while (*p == ' ' || *p == '\t')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }
        if (argc + 1 >= argv_size)
        {
            break;
        }

        char quote = '\0';
        if (*p == '"' || *p == '\'')
        {
            quote = *p++;
        }
        argv[argc++] = p;

        while (*p != '\0')
        {
            if (quote != '\0')
            {
                if (*p == quote)
                {
                    *p++ = '\0';
                    break;
                }
            }
            else if (*p == ' ' || *p == '\t')
            {
                *p++ = '\0';
                break;
            }
            if (*p != '\0')
            {
                p++;
            }
        }
    }

    if (argc < argv_size)
    {
        argv[argc] = NULL;
    }
    return argc;
}

esp_err_t esp_console_run(const char *cmdline, int *cmd_ret)
{
    if (cmdline == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char *copy = strdup(cmdline);
    if (copy == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    char *argv[MOCK_MAX_ARGS];
    size_t argc = esp_console_split_argv(copy, argv, MOCK_MAX_ARGS);
    if (argc == 0)
    {
        free(copy);
        return ESP_ERR_INVALID_ARG;
    }

    int ret = mock_console_run_cmd(argv[0], (int)argc, argv);
    free(copy);
    if (ret < 0)
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (cmd_ret != NULL)
    {
        *cmd_ret = ret;
    }
    return ESP_OK;
}
