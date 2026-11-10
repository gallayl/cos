#include "shell_script.h"

#include "berry_runtime.h"
#include "filesystem.h"
#include "shell.h"
#include "shell_redirection.h"

#include "esp_console.h"
#include "esp_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "shell_script";

#define PATH_FILE "/flash/.cos_path"
#define PATH_MAX_ENTRIES 8
#define PATH_FILE_MAX (PATH_MAX_ENTRIES * (VFS_PATH_MAX + 1))
#define SCRIPT_DISPATCH_ARGS 16

static char s_path_dirs[PATH_MAX_ENTRIES][VFS_PATH_MAX];
static size_t s_path_count;

static bool is_explicit_script_token(const char *token)
{
    return token != NULL && strchr(token, '/') != NULL;
}

static bool has_be_extension(const char *name)
{
    if (name == NULL)
    {
        return false;
    }
    size_t len = strlen(name);
    return len >= 3 && strcmp(name + len - 3, ".be") == 0;
}

static esp_err_t normalize_path_dir(const char *input, char *normalized, size_t len)
{
    if (input == NULL || normalized == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = shell_resolve_relative(input, normalized, len);
    if (err != ESP_OK)
    {
        return err;
    }

    if (strcmp(normalized, "/") != 0)
    {
        size_t nlen = strlen(normalized);
        if (nlen > 1 && normalized[nlen - 1] == '/')
        {
            normalized[nlen - 1] = '\0';
        }
    }
    return ESP_OK;
}

static int find_path_entry(const char *normalized)
{
    for (size_t i = 0; i < s_path_count; i++)
    {
        if (strcmp(s_path_dirs[i], normalized) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

static esp_err_t save_path_config(void)
{
    char contents[PATH_FILE_MAX];
    size_t length = 0;

    for (size_t i = 0; i < s_path_count; i++)
    {
        int written = snprintf(contents + length, sizeof(contents) - length, "%s\n", s_path_dirs[i]);
        if (written < 0 || (size_t)written >= sizeof(contents) - length)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        length += (size_t)written;
    }

    return vfs_write_file(PATH_FILE, contents, length);
}

static void load_path_config(void)
{
    s_path_count = 0;
    if (!vfs_exists(PATH_FILE))
    {
        return;
    }

    char contents[PATH_FILE_MAX + 1];
    size_t length = 0;
    esp_err_t err = vfs_read_file(PATH_FILE, contents, sizeof(contents) - 1, &length);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Could not read %s: %s", PATH_FILE, esp_err_to_name(err));
        return;
    }
    contents[length] = '\0';

    char *line = contents;
    while (*line != '\0' && s_path_count < PATH_MAX_ENTRIES)
    {
        char *end = strchr(line, '\n');
        if (end != NULL)
        {
            *end = '\0';
        }
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\r')
        {
            line[--line_len] = '\0';
        }

        if (line_len > 0)
        {
            char normalized[VFS_PATH_MAX];
            if (normalize_path_dir(line, normalized, sizeof(normalized)) == ESP_OK && normalized[0] == '/' &&
                find_path_entry(normalized) < 0)
            {
                strncpy(s_path_dirs[s_path_count], normalized, VFS_PATH_MAX - 1);
                s_path_dirs[s_path_count][VFS_PATH_MAX - 1] = '\0';
                s_path_count++;
            }
            else
            {
                ESP_LOGW(TAG, "Ignoring invalid path entry: %s", line);
            }
        }

        if (end == NULL)
        {
            break;
        }
        line = end + 1;
    }
}

static void print_path_usage(void)
{
    printf("Usage: path [list|add <directory>|remove <directory>|clear]\n");
}

static int cmd_path(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "list") == 0))
    {
        if (s_path_count == 0)
        {
            printf("(empty)\n");
        }
        else
        {
            for (size_t i = 0; i < s_path_count; i++)
            {
                printf("%s\n", s_path_dirs[i]);
            }
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0)
    {
        char saved[PATH_MAX_ENTRIES][VFS_PATH_MAX];
        size_t saved_count = s_path_count;
        memcpy(saved, s_path_dirs, sizeof(saved));
        s_path_count = 0;
        memset(s_path_dirs, 0, sizeof(s_path_dirs));
        esp_err_t err = save_path_config();
        if (err != ESP_OK)
        {
            s_path_count = saved_count;
            memcpy(s_path_dirs, saved, sizeof(saved));
            printf("path: could not save configuration: %s\n", esp_err_to_name(err));
            return 1;
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "add") == 0)
    {
        char normalized[VFS_PATH_MAX];
        esp_err_t err = normalize_path_dir(argv[2], normalized, sizeof(normalized));
        if (err != ESP_OK)
        {
            printf("path: invalid directory '%s': %s\n", argv[2], esp_err_to_name(err));
            return 1;
        }
        if (!vfs_is_directory(normalized))
        {
            printf("path: '%s' is not an available directory\n", normalized);
            return 1;
        }
        if (find_path_entry(normalized) >= 0)
        {
            return 0;
        }
        if (s_path_count >= PATH_MAX_ENTRIES)
        {
            printf("path: maximum of %u directories reached\n", (unsigned)PATH_MAX_ENTRIES);
            return 1;
        }

        char saved[PATH_MAX_ENTRIES][VFS_PATH_MAX];
        size_t saved_count = s_path_count;
        memcpy(saved, s_path_dirs, sizeof(saved));
        strncpy(s_path_dirs[s_path_count], normalized, VFS_PATH_MAX - 1);
        s_path_dirs[s_path_count][VFS_PATH_MAX - 1] = '\0';
        s_path_count++;
        err = save_path_config();
        if (err != ESP_OK)
        {
            s_path_count = saved_count;
            memcpy(s_path_dirs, saved, sizeof(saved));
            printf("path: could not save configuration: %s\n", esp_err_to_name(err));
            return 1;
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "remove") == 0)
    {
        char normalized[VFS_PATH_MAX];
        esp_err_t err = normalize_path_dir(argv[2], normalized, sizeof(normalized));
        if (err != ESP_OK)
        {
            printf("path: invalid directory '%s': %s\n", argv[2], esp_err_to_name(err));
            return 1;
        }
        int index = find_path_entry(normalized);
        if (index < 0)
        {
            printf("path: '%s' not found\n", normalized);
            return 1;
        }

        char saved[PATH_MAX_ENTRIES][VFS_PATH_MAX];
        size_t saved_count = s_path_count;
        memcpy(saved, s_path_dirs, sizeof(saved));
        memmove(&s_path_dirs[index], &s_path_dirs[index + 1],
                (s_path_count - (size_t)index - 1) * sizeof(s_path_dirs[0]));
        memset(&s_path_dirs[s_path_count - 1], 0, sizeof(s_path_dirs[0]));
        s_path_count--;
        err = save_path_config();
        if (err != ESP_OK)
        {
            s_path_count = saved_count;
            memcpy(s_path_dirs, saved, sizeof(saved));
            printf("path: could not save configuration: %s\n", esp_err_to_name(err));
            return 1;
        }
        return 0;
    }

    print_path_usage();
    return 1;
}

static esp_err_t join_script_candidate(const char *dir, const char *name, char *out, size_t len)
{
    if (strcmp(dir, "/") == 0)
    {
        return snprintf(out, len, "/%s", name) < (int)len ? ESP_OK : ESP_ERR_INVALID_SIZE;
    }
    return snprintf(out, len, "%s/%s", dir, name) < (int)len ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t script_candidate_name(const char *token, bool explicit_path, char *name, size_t len)
{
    if (token == NULL || name == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (has_be_extension(token))
    {
        return snprintf(name, len, "%s", token) < (int)len ? ESP_OK : ESP_ERR_INVALID_SIZE;
    }

    return snprintf(name, len, "%s.be", token) < (int)len ? ESP_OK : ESP_ERR_INVALID_SIZE;
    (void)explicit_path;
}

static esp_err_t resolve_explicit_script(const char *token, char *resolved, size_t len, bool *found)
{
    *found = false;
    char with_ext[VFS_PATH_MAX];
    esp_err_t err = script_candidate_name(token, true, with_ext, sizeof(with_ext));
    if (err != ESP_OK)
    {
        return err;
    }
    err = shell_resolve_relative(with_ext, resolved, len);
    if (err != ESP_OK)
    {
        return err;
    }
    if (vfs_exists(resolved))
    {
        if (vfs_is_directory(resolved))
        {
            return ESP_ERR_INVALID_ARG;
        }
        *found = true;
        return ESP_OK;
    }
    return ESP_OK;
}

static esp_err_t resolve_script_command(int argc, char **argv, bool explicit_path, char *resolved, size_t len,
                                        bool *found)
{
    (void)argc;
    *found = false;
    if (argv == NULL || argv[0] == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (explicit_path)
    {
        return resolve_explicit_script(argv[0], resolved, len, found);
    }

    char name[VFS_PATH_MAX];
    esp_err_t err = script_candidate_name(argv[0], false, name, sizeof(name));
    if (err != ESP_OK)
    {
        return err;
    }

    char candidate[VFS_PATH_MAX];
    err = join_script_candidate(shell_get_cwd(), name, candidate, sizeof(candidate));
    if (err != ESP_OK)
    {
        return err;
    }
    if (vfs_exists(candidate) && !vfs_is_directory(candidate))
    {
        strncpy(resolved, candidate, len - 1);
        resolved[len - 1] = '\0';
        *found = true;
        return ESP_OK;
    }

    for (size_t i = 0; i < s_path_count; i++)
    {
        err = join_script_candidate(s_path_dirs[i], name, candidate, sizeof(candidate));
        if (err != ESP_OK)
        {
            return err;
        }
        if (vfs_exists(candidate))
        {
            if (vfs_is_directory(candidate))
            {
                continue;
            }
            strncpy(resolved, candidate, len - 1);
            resolved[len - 1] = '\0';
            *found = true;
            return ESP_OK;
        }
    }

    return ESP_OK;
}

esp_err_t shell_script_try_exec(int argc, char **argv, bool explicit_path, int *ret, bool *handled)
{
    if (argv == NULL || argv[0] == NULL || ret == NULL || handled == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *handled = false;
    *ret = 1;

    char resolved[VFS_PATH_MAX];
    bool found = false;
    esp_err_t err = resolve_script_command(argc, argv, explicit_path, resolved, sizeof(resolved), &found);
    if (err != ESP_OK)
    {
        printf("%s: %s\n", argv[0], esp_err_to_name(err));
        *handled = true;
        return err;
    }

    if (!found)
    {
        if (explicit_path)
        {
            printf("%s: script not found\n", argv[0]);
            *handled = true;
            return ESP_ERR_NOT_FOUND;
        }
        return ESP_OK;
    }

    err = berry_runtime_init();
    if (err != ESP_OK)
    {
        printf("%s: Berry initialization failed (%s)\n", argv[0], esp_err_to_name(err));
        *handled = true;
        return err;
    }

    int exit_status = 1;
    err = berry_runtime_run_file(resolved, argc - 1, argv + 1, &exit_status);
    *handled = true;
    if (err == ESP_OK)
    {
        *ret = exit_status;
        return ESP_OK;
    }

    printf("%s: %s\n", argv[0], esp_err_to_name(err));
    *ret = 1;
    return err;
}

static esp_err_t shell_dispatch_command_line(const char *line, int *ret)
{
    esp_err_t err = esp_console_run(line, ret);
    if (err != ESP_ERR_NOT_FOUND)
    {
        return err;
    }

    char *copy = strdup(line);
    if (copy == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    char *argv[SCRIPT_DISPATCH_ARGS];
    size_t argc = esp_console_split_argv(copy, argv, SCRIPT_DISPATCH_ARGS);
    if (argc == 0)
    {
        free(copy);
        return ESP_ERR_INVALID_ARG;
    }

    bool handled = false;
    err = shell_script_try_exec((int)argc, argv, is_explicit_script_token(argv[0]), ret, &handled);
    free(copy);
    if (!handled)
    {
        return ESP_ERR_NOT_FOUND;
    }

    /*
     * Script execution or a resolver-reported error was handled. Return ESP_OK
     * so callers do not print "Unknown command"; *ret carries status (1 for
     * reported errors).
     */
    (void)err;
    return ESP_OK;
}

esp_err_t shell_dispatch_expanded_line(const char *line, int *ret)
{
    if (line == NULL || ret == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *ret = 0;

    shell_pipeline_info_t pipeline;
    esp_err_t pipe_err = shell_parse_pipeline(line, &pipeline);
    if (pipe_err != ESP_OK)
    {
        printf("pipes: parse failed (%s)\n", esp_err_to_name(pipe_err));
        *ret = 1;
        return ESP_OK;
    }
    if (pipeline.status == SHELL_PIPELINE_INVALID)
    {
        printf("pipes: invalid syntax\n");
        *ret = 1;
        return ESP_OK;
    }
    if (pipeline.status == SHELL_PIPELINE_UNSUPPORTED)
    {
        printf("pipes: not supported yet\n");
        *ret = 1;
        return ESP_OK;
    }

    shell_redirection_t redir;
    esp_err_t parse_err = shell_parse_redirection(line, &redir);
    if (parse_err != ESP_OK)
    {
        if (parse_err == ESP_ERR_INVALID_ARG)
        {
            printf("redirection: invalid or unsupported syntax\n");
        }
        else if (parse_err == ESP_ERR_INVALID_SIZE)
        {
            printf("redirection: command or target is too long\n");
        }
        else
        {
            printf("redirection: parse failed (%s)\n", esp_err_to_name(parse_err));
        }
        *ret = 1;
        return ESP_OK;
    }

    if (redir.type == SHELL_REDIRECT_NONE)
    {
        return shell_dispatch_command_line(line, ret);
    }

    shell_redirect_state_t redirect_state;
    esp_err_t redirect_err = shell_redirect_begin(&redir, &redirect_state);
    if (redirect_err != ESP_OK)
    {
        printf("redirection: cannot redirect stdout to '%s': %s\n", redir.target, esp_err_to_name(redirect_err));
        *ret = 1;
        return ESP_OK;
    }

    esp_err_t err = shell_dispatch_command_line(redir.command, ret);
    fflush(stdout);
    shell_redirect_end(&redirect_state);

    if (err != ESP_OK)
    {
        /* Infrastructure or unknown-command errors were not printed while redirected. */
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unknown command: %s\n", redir.command);
            *ret = 1;
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            *ret = (*ret != 0) ? *ret : 1;
        }
        else
        {
            printf("redirection: command dispatch failed (%s)\n", esp_err_to_name(err));
            *ret = 1;
        }
        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t shell_script_init(void)
{
    memset(s_path_dirs, 0, sizeof(s_path_dirs));
    s_path_count = 0;

    const esp_console_cmd_t path_command = {
        .command = "path",
        .help = "List or configure script search directories",
        .hint = "[list|add <directory>|remove <directory>|clear]",
        .func = &cmd_path,
    };
    esp_err_t err = esp_console_cmd_register(&path_command);
    if (err != ESP_OK)
    {
        return err;
    }

    load_path_config();
    ESP_LOGI(TAG, "Script path ready (%u configured director%s)", (unsigned)s_path_count,
             s_path_count == 1 ? "y" : "ies");
    return ESP_OK;
}
