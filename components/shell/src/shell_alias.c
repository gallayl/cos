#include "shell_alias.h"

#include "esp_console.h"
#include "esp_log.h"
#include "filesystem.h"
#include "linenoise/linenoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "shell_alias";

#define ALIAS_FILE "/flash/.cos_aliases"
#define ALIAS_MAX 8
#define ALIAS_NAME_MAX 16
#define ALIAS_COMMAND_MAX 32
#define ALIAS_FILE_MAX (ALIAS_MAX * (ALIAS_NAME_MAX + ALIAS_COMMAND_MAX + 2))

typedef struct
{
    char name[ALIAS_NAME_MAX];
    char command[ALIAS_COMMAND_MAX];
} shell_alias_t;

static shell_alias_t s_aliases[ALIAS_MAX];
static size_t s_alias_count;

static bool command_exists(const char *command)
{
    if (command == NULL || command[0] == '\0')
    {
        return false;
    }

    linenoiseCompletions completions = {0};
    esp_console_get_completion(command, &completions);
    bool found = false;
    for (size_t i = 0; i < completions.len; i++)
    {
        if (strcmp(completions.cvec[i], command) == 0)
        {
            found = true;
        }
        free(completions.cvec[i]);
    }
    free((void *)completions.cvec);
    return found;
}

static int find_alias(const shell_alias_t *aliases, size_t count, const char *name)
{
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(aliases[i].name, name) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

static bool valid_name(const char *name)
{
    if (name == NULL || name[0] == '\0' || name[0] == '-' || name[0] == '_' || strlen(name) >= ALIAS_NAME_MAX)
    {
        return false;
    }
    for (const char *ch = name; *ch != '\0'; ch++)
    {
        if ((*ch < 'a' || *ch > 'z') && (*ch < 'A' || *ch > 'Z') && (*ch < '0' || *ch > '9') && *ch != '_' &&
            *ch != '-')
        {
            return false;
        }
    }
    return true;
}

static bool valid_command(const shell_alias_t *aliases, size_t count, const char *command)
{
    return command != NULL && command[0] != '\0' && strlen(command) < ALIAS_COMMAND_MAX &&
           find_alias(aliases, count, command) < 0 && command_exists(command);
}

static esp_err_t save_aliases(const shell_alias_t *aliases, size_t count)
{
    char contents[ALIAS_FILE_MAX];
    size_t length = 0;

    for (size_t i = 0; i < count; i++)
    {
        int written =
            snprintf(contents + length, sizeof(contents) - length, "%s=%s\n", aliases[i].name, aliases[i].command);
        if (written < 0 || (size_t)written >= sizeof(contents) - length)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        length += (size_t)written;
    }

    return vfs_write_file(ALIAS_FILE, contents, length);
}

static int cmd_alias(int argc, char **argv)
{
    if (argc == 1)
    {
        for (size_t i = 0; i < s_alias_count; i++)
        {
            printf("%s=%s\n", s_aliases[i].name, s_aliases[i].command);
        }
        return 0;
    }

    if (argc != 3 || !valid_name(argv[1]) || !valid_command(s_aliases, s_alias_count, argv[2]))
    {
        printf("Usage: alias [<name> <command>]\n");
        return 1;
    }
    if (command_exists(argv[1]))
    {
        printf("alias: '%s' is already a command\n", argv[1]);
        return 1;
    }

    shell_alias_t updated[ALIAS_MAX];
    memcpy(updated, s_aliases, sizeof(updated));
    int index = find_alias(updated, s_alias_count, argv[1]);
    if (index < 0)
    {
        if (s_alias_count == ALIAS_MAX)
        {
            printf("alias: maximum of %u aliases reached\n", (unsigned)ALIAS_MAX);
            return 1;
        }
        index = (int)s_alias_count;
        strncpy(updated[index].name, argv[1], sizeof(updated[index].name) - 1);
        updated[index].name[sizeof(updated[index].name) - 1] = '\0';
    }
    strncpy(updated[index].command, argv[2], sizeof(updated[index].command) - 1);
    updated[index].command[sizeof(updated[index].command) - 1] = '\0';

    esp_err_t err = save_aliases(updated, index == (int)s_alias_count ? s_alias_count + 1 : s_alias_count);
    if (err != ESP_OK)
    {
        printf("alias: could not save configuration: %s\n", esp_err_to_name(err));
        return 1;
    }

    if (index == (int)s_alias_count)
    {
        s_alias_count++;
    }
    memcpy(s_aliases, updated, sizeof(s_aliases));
    return 0;
}

static int cmd_unalias(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: unalias <name>\n");
        return 1;
    }

    int index = find_alias(s_aliases, s_alias_count, argv[1]);
    if (index < 0)
    {
        printf("unalias: '%s' not found\n", argv[1]);
        return 1;
    }

    shell_alias_t updated[ALIAS_MAX];
    memcpy(updated, s_aliases, sizeof(updated));
    memmove(&updated[index], &updated[index + 1], (s_alias_count - (size_t)index - 1) * sizeof(updated[0]));
    memset(&updated[s_alias_count - 1], 0, sizeof(updated[0]));

    esp_err_t err = save_aliases(updated, s_alias_count - 1);
    if (err != ESP_OK)
    {
        printf("unalias: could not save configuration: %s\n", esp_err_to_name(err));
        return 1;
    }

    s_alias_count--;
    memcpy(s_aliases, updated, sizeof(s_aliases));
    return 0;
}

esp_err_t shell_alias_init(void)
{
    memset(s_aliases, 0, sizeof(s_aliases));
    s_alias_count = 0;

    const esp_console_cmd_t alias_command = {
        .command = "alias",
        .help = "List or define a shell alias",
        .hint = "[<name> <command>]",
        .func = &cmd_alias,
    };
    esp_err_t err = esp_console_cmd_register(&alias_command);
    if (err != ESP_OK)
    {
        return err;
    }

    const esp_console_cmd_t unalias_command = {
        .command = "unalias",
        .help = "Remove a shell alias",
        .hint = "<name>",
        .func = &cmd_unalias,
    };
    err = esp_console_cmd_register(&unalias_command);
    if (err != ESP_OK)
    {
        return err;
    }

    if (!vfs_exists(ALIAS_FILE))
    {
        return ESP_OK;
    }

    char contents[ALIAS_FILE_MAX + 1];
    size_t length = 0;
    err = vfs_read_file(ALIAS_FILE, contents, sizeof(contents) - 1, &length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read alias configuration: %s", esp_err_to_name(err));
        return ESP_OK;
    }
    if (length >= ALIAS_FILE_MAX)
    {
        ESP_LOGE(TAG, "Alias configuration exceeds %u bytes", (unsigned)ALIAS_FILE_MAX);
        return ESP_OK;
    }
    contents[length] = '\0';

    char *line = contents;
    while (*line != '\0')
    {
        char *end = strchr(line, '\n');
        if (end != NULL)
        {
            *end = '\0';
        }
        size_t line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\r')
        {
            line[line_length - 1] = '\0';
        }
        char *separator = strchr(line, '=');
        if (separator != NULL)
        {
            *separator = '\0';
            const char *command = separator + 1;
            if (valid_name(line) && valid_command(s_aliases, s_alias_count, command) && !command_exists(line) &&
                s_alias_count < ALIAS_MAX && find_alias(s_aliases, s_alias_count, line) < 0)
            {
                strncpy(s_aliases[s_alias_count].name, line, sizeof(s_aliases[0].name) - 1);
                strncpy(s_aliases[s_alias_count].command, command, sizeof(s_aliases[0].command) - 1);
                s_alias_count++;
            }
            else
            {
                ESP_LOGW(TAG, "Ignoring invalid alias configuration entry");
            }
        }
        else
        {
            ESP_LOGW(TAG, "Ignoring malformed alias configuration entry");
        }
        if (end == NULL)
        {
            break;
        }
        line = end + 1;
    }

    return ESP_OK;
}

esp_err_t shell_alias_expand(const char *line, char *expanded, size_t expanded_size)
{
    if (line == NULL || expanded == NULL || expanded_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *command_start = line;
    while (*command_start == ' ' || *command_start == '\t')
    {
        command_start++;
    }
    const char *command_end = command_start;
    while (*command_end != '\0' && *command_end != ' ' && *command_end != '\t')
    {
        command_end++;
    }

    size_t command_length = (size_t)(command_end - command_start);
    if (command_length > 0 && command_length < ALIAS_NAME_MAX)
    {
        char name[ALIAS_NAME_MAX];
        memcpy(name, command_start, command_length);
        name[command_length] = '\0';
        int index = find_alias(s_aliases, s_alias_count, name);
        if (index >= 0)
        {
            size_t prefix_length = (size_t)(command_start - line);
            size_t suffix_length = strlen(command_end);
            size_t replacement_length = strlen(s_aliases[index].command);
            if (prefix_length + replacement_length + suffix_length >= expanded_size)
            {
                return ESP_ERR_INVALID_SIZE;
            }
            memcpy(expanded, line, prefix_length);
            memcpy(expanded + prefix_length, s_aliases[index].command, replacement_length);
            memcpy(expanded + prefix_length + replacement_length, command_end, suffix_length + 1);
            return ESP_OK;
        }
    }

    size_t length = strlen(line);
    if (length >= expanded_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(expanded, line, length + 1);
    return ESP_OK;
}
