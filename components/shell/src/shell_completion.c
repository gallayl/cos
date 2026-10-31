#include "shell_completion.h"

#include "filesystem.h"
#include "shell.h"

#include "esp_console.h"
#include "linenoise/linenoise.h"

#include <stdlib.h>
#include <string.h>

static void free_completions(linenoiseCompletions *completions)
{
    for (size_t i = 0; i < completions->len; i++)
    {
        free(completions->cvec[i]);
    }
    free((void *)completions->cvec);
}

static esp_err_t replace_token(char *line, size_t capacity, size_t token_start, const char *replacement,
                               const char *suffix, bool *changed)
{
    size_t line_len = strlen(line);
    size_t replacement_len = strlen(replacement);
    size_t suffix_len = strlen(suffix);
    size_t result_len = token_start + replacement_len + suffix_len;
    if (result_len >= capacity)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    if (line_len == result_len && strncmp(line + token_start, replacement, replacement_len) == 0 &&
        strcmp(line + token_start + replacement_len, suffix) == 0)
    {
        return ESP_OK;
    }

    memcpy(line + token_start, replacement, replacement_len);
    memcpy(line + token_start + replacement_len, suffix, suffix_len);
    line[result_len] = '\0';
    *changed = true;
    return ESP_OK;
}

static esp_err_t complete_command(char *line, size_t capacity, bool *changed)
{
    linenoiseCompletions completions = {0};
    esp_console_get_completion(line, &completions);
    if (completions.len == 0)
    {
        free_completions(&completions);
        return ESP_OK;
    }

    size_t common_len = strlen(completions.cvec[0]);
    for (size_t i = 1; i < completions.len; i++)
    {
        size_t match_len = strlen(completions.cvec[i]);
        if (match_len < common_len)
        {
            common_len = match_len;
        }
        size_t j = 0;
        while (j < common_len && completions.cvec[0][j] == completions.cvec[i][j])
        {
            j++;
        }
        common_len = j;
    }

    const char *suffix = (completions.len == 1) ? " " : "";
    char replacement[VFS_PATH_MAX];
    if (common_len >= sizeof(replacement))
    {
        free_completions(&completions);
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(replacement, completions.cvec[0], common_len);
    replacement[common_len] = '\0';

    esp_err_t err = (common_len <= strlen(line) && suffix[0] == '\0')
                        ? ESP_OK
                        : replace_token(line, capacity, 0, replacement, suffix, changed);
    free_completions(&completions);
    return err;
}

static size_t count_words(const char *line, size_t end)
{
    size_t count = 0;
    bool in_word = false;
    for (size_t i = 0; i < end; i++)
    {
        if (line[i] == ' ')
        {
            in_word = false;
        }
        else if (!in_word)
        {
            count++;
            in_word = true;
        }
    }
    return count;
}

static bool is_path_argument(const char *line, size_t argument_index)
{
    size_t command_start = 0;
    while (line[command_start] == ' ')
    {
        command_start++;
    }
    size_t command_end = command_start;
    while (line[command_end] != '\0' && line[command_end] != ' ')
    {
        command_end++;
    }
    size_t command_len = command_end - command_start;

    static const char *const SINGLE_PATH_COMMANDS[] = {
        "cd", "ls", "dir", "mkdir", "md", "rm", "del", "rmdir", "cat", "type", "touch", "hexdump",
    };
    if (argument_index == 1)
    {
        for (size_t i = 0; i < sizeof(SINGLE_PATH_COMMANDS) / sizeof(SINGLE_PATH_COMMANDS[0]); i++)
        {
            if (strlen(SINGLE_PATH_COMMANDS[i]) == command_len &&
                strncmp(line + command_start, SINGLE_PATH_COMMANDS[i], command_len) == 0)
            {
                return true;
            }
        }
    }
    if (argument_index == 1 || argument_index == 2)
    {
        if ((command_len == 2 && strncmp(line + command_start, "cp", 2) == 0) ||
            (command_len == 2 && strncmp(line + command_start, "mv", 2) == 0))
        {
            return true;
        }
    }

    if (command_len == 5 && strncmp(line + command_start, "berry", 5) == 0 && argument_index == 2)
    {
        size_t subcommand_start = command_end;
        while (line[subcommand_start] == ' ')
        {
            subcommand_start++;
        }
        if (strncmp(line + subcommand_start, "run", 3) == 0 &&
            (line[subcommand_start + 3] == '\0' || line[subcommand_start + 3] == ' '))
        {
            return true;
        }
    }

    return false;
}

static esp_err_t complete_path(char *line, size_t capacity, size_t token_start, bool *changed)
{
    char token[VFS_PATH_MAX];
    size_t token_len = strlen(line) - token_start;
    if (token_len >= sizeof(token))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(token, line + token_start, token_len);
    token[token_len] = '\0';

    const char *slash = strrchr(token, '/');
    size_t path_prefix_len = 0;
    char parent_input[VFS_PATH_MAX];
    const char *name_prefix = token;
    if (slash != NULL)
    {
        path_prefix_len = (size_t)(slash - token) + 1;
        name_prefix = slash + 1;
        if (slash == token)
        {
            strcpy(parent_input, "/");
        }
        else
        {
            size_t parent_len = (size_t)(slash - token);
            memcpy(parent_input, token, parent_len);
            parent_input[parent_len] = '\0';
        }
    }
    else
    {
        strcpy(parent_input, ".");
    }

    char parent_path[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(parent_input, parent_path, sizeof(parent_path));
    if (err != ESP_OK)
    {
        return ESP_OK;
    }

    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(*entries));
    if (entries == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    size_t entry_count = 0;
    err = vfs_list_dir(parent_path, entries, VFS_ENTRIES_MAX, &entry_count);
    if (err == ESP_ERR_NOT_FOUND)
    {
        free(entries);
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        free(entries);
        return err;
    }

    size_t prefix_len = strlen(name_prefix);
    size_t match_count = 0;
    size_t common_len = 0;
    size_t match_index = 0;
    for (size_t i = 0; i < entry_count; i++)
    {
        if (strncmp(entries[i].name, name_prefix, prefix_len) != 0 || strchr(entries[i].name, ' ') != NULL ||
            strchr(entries[i].name, '\t') != NULL)
        {
            continue;
        }
        if (match_count == 0)
        {
            common_len = strlen(entries[i].name);
            match_index = i;
        }
        else
        {
            size_t name_len = strlen(entries[i].name);
            if (name_len < common_len)
            {
                common_len = name_len;
            }
            size_t j = 0;
            while (j < common_len && entries[match_index].name[j] == entries[i].name[j])
            {
                j++;
            }
            common_len = j;
        }
        match_count++;
    }

    if (match_count == 0 || (match_count > 1 && common_len <= prefix_len))
    {
        free(entries);
        return ESP_OK;
    }

    char replacement[VFS_PATH_MAX];
    const char *suffix = "";
    const char *matched_name = entries[match_index].name;
    size_t replacement_name_len = (match_count == 1) ? strlen(matched_name) : common_len;
    if (path_prefix_len + replacement_name_len >= sizeof(replacement))
    {
        free(entries);
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(replacement, token, path_prefix_len);
    memcpy(replacement + path_prefix_len, matched_name, replacement_name_len);
    replacement[path_prefix_len + replacement_name_len] = '\0';
    if (match_count == 1)
    {
        if (entries[match_index].is_dir)
        {
            suffix = "/";
        }
        else
        {
            suffix = " ";
        }
    }

    err = replace_token(line, capacity, token_start, replacement, suffix, changed);
    free(entries);
    return err;
}

esp_err_t shell_complete_line(char *line, size_t capacity, bool *changed)
{
    if (line == NULL || capacity == 0 || changed == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *changed = false;

    size_t line_len = strlen(line);
    size_t token_start = line_len;
    while (token_start > 0 && line[token_start - 1] != ' ')
    {
        token_start--;
    }

    if (token_start == 0)
    {
        return complete_command(line, capacity, changed);
    }

    size_t argument_index = count_words(line, token_start);
    if (!is_path_argument(line, argument_index))
    {
        return ESP_OK;
    }
    return complete_path(line, capacity, token_start, changed);
}
