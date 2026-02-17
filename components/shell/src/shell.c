#include "shell.h"
#include "filesystem.h"

#include "console_private.h"
#include "esp_console.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <sys/cdefs.h>

static const char *const TAG = "shell";

#define SHELL_CWD_MAX 64
#define SHELL_PROMPT_MAX 80

static char s_cwd[SHELL_CWD_MAX] = "/flash";
static char s_prompt[SHELL_PROMPT_MAX] = "COS/flash> ";
static esp_console_repl_t *s_repl = NULL;

/* Defined in shell_fs_cmds.c */
void shell_register_fs_commands(void);
/* Defined in shell_sd_cmds.c */
void shell_register_sd_commands(void);
/* Defined in shell_info_cmds.c */
void shell_register_info_commands(void);

static void sync_repl_prompt(void)
{
    if (s_repl == NULL)
    {
        return;
    }

    esp_console_repl_com_t *repl_com = __containerof(s_repl, esp_console_repl_com_t, repl_core);

    if (strlen(s_prompt) < CONSOLE_PROMPT_MAX_LEN)
    {
        snprintf(repl_com->prompt, CONSOLE_PROMPT_MAX_LEN, "%s", s_prompt);
        return;
    }

    /* Path too long for REPL buffer – elide middle segments: COS/<first>/.../<last> > */
    const char *second_slash = strchr(s_cwd + 1, '/');
    const char *last_slash = strrchr(s_cwd, '/');

    if (second_slash != NULL && last_slash != NULL && second_slash < last_slash)
    {
        int first_len = (int)(second_slash - s_cwd);
        snprintf(repl_com->prompt, CONSOLE_PROMPT_MAX_LEN, "COS%.*s/...%s> ", first_len, s_cwd, last_slash);
    }
    else
    {
        snprintf(repl_com->prompt, CONSOLE_PROMPT_MAX_LEN, "%s", s_prompt);
    }
}

static void update_prompt(void)
{
    if (strcmp(s_cwd, "/") == 0)
    {
        snprintf(s_prompt, sizeof(s_prompt), "COS> ");
    }
    else
    {
        snprintf(s_prompt, sizeof(s_prompt), "COS%s> ", s_cwd);
    }

    sync_repl_prompt();
}

const char *shell_get_cwd(void)
{
    return s_cwd;
}

const char *shell_get_prompt(void)
{
    return s_prompt;
}

esp_err_t shell_set_cwd(const char *path)
{
    if (path == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!vfs_is_directory(path))
    {
        return ESP_ERR_NOT_FOUND;
    }

    if (strlen(path) >= sizeof(s_cwd))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    strncpy(s_cwd, path, sizeof(s_cwd) - 1);
    s_cwd[sizeof(s_cwd) - 1] = '\0';

    /* Strip trailing slash (except for root "/") */
    size_t len = strlen(s_cwd);
    if (len > 1 && s_cwd[len - 1] == '/')
    {
        s_cwd[len - 1] = '\0';
    }

    update_prompt();
    return ESP_OK;
}

esp_err_t shell_resolve_relative(const char *input, char *abs_path, size_t len)
{
    if (input == NULL || abs_path == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char working[VFS_PATH_MAX];

    if (input[0] == '/')
    {
        /* Absolute path */
        strncpy(working, input, sizeof(working) - 1);
        working[sizeof(working) - 1] = '\0';
    }
    else
    {
        /* Relative to cwd */
        if (strcmp(s_cwd, "/") == 0)
        {
            snprintf(working, sizeof(working), "/%s", input);
        }
        else
        {
            snprintf(working, sizeof(working), "%s/%s", s_cwd, input);
        }
    }

    /* Normalize: resolve "." and ".." components */
    char *components[32];
    int depth = 0;

    char *saveptr = NULL;
    // NOLINTNEXTLINE(performance-no-int-to-ptr) -- strtok_r newlib implementation artifact
    char *token = strtok_r(working, "/", &saveptr);
    while (token != NULL)
    {
        if (strcmp(token, ".") == 0)
        {
            /* Skip */
        }
        else if (strcmp(token, "..") == 0)
        {
            if (depth > 0)
            {
                depth--;
            }
        }
        else
        {
            if (depth < 32)
            {
                components[depth++] = token;
            }
        }
        // NOLINTNEXTLINE(performance-no-int-to-ptr) -- strtok_r newlib implementation artifact
        token = strtok_r(NULL, "/", &saveptr);
    }

    if (depth == 0)
    {
        strncpy(abs_path, "/", len);
        return ESP_OK;
    }

    abs_path[0] = '\0';
    for (int i = 0; i < depth; i++)
    {
        size_t cur_len = strlen(abs_path);
        snprintf(abs_path + cur_len, len - cur_len, "/%s", components[i]);
    }

    return ESP_OK;
}

esp_err_t shell_init(void)
{
    ESP_LOGI(TAG, "Initializing shell...");

    shell_register_fs_commands();
    shell_register_sd_commands();
    shell_register_info_commands();

    update_prompt();

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.task_stack_size = 8192;
    repl_config.prompt = s_prompt;

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create REPL: %s", esp_err_to_name(err));
        return err;
    }

    s_repl = repl;

    err = esp_console_start_repl(repl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start REPL: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Shell ready");
    return ESP_OK;
}
