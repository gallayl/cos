#include "shell_redirection.h"
#include "shell.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#define shell_close _close
#define shell_dup _dup
#define shell_dup2 _dup2
#define shell_fileno _fileno
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#else
#include <unistd.h>
#define shell_close close
#define shell_fileno fileno

static int shell_dup(int fd)
{
#if defined(F_DUPFD_CLOEXEC)
    int duplicated = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (duplicated >= 0)
    {
        return duplicated;
    }
#endif
#if defined(F_DUPFD)
    duplicated = fcntl(fd, F_DUPFD, 0);
    if (duplicated >= 0)
    {
        return duplicated;
    }
#endif
    return -1;
}

static int shell_dup2(int oldfd, int newfd)
{
    if (oldfd == newfd)
    {
        return newfd;
    }
    if (shell_close(newfd) < 0)
    {
        /* Ignore close errors; fcntl may still succeed if fd was not open. */
    }
#if defined(F_DUPFD_CLOEXEC)
    {
        int duplicated = fcntl(oldfd, F_DUPFD_CLOEXEC, newfd);
        if (duplicated == newfd)
        {
            return duplicated;
        }
        if (duplicated >= 0)
        {
            (void)shell_close(duplicated);
        }
    }
#endif
#if defined(F_DUPFD)
    {
        int duplicated = fcntl(oldfd, F_DUPFD, newfd);
        if (duplicated == newfd)
        {
            return duplicated;
        }
        if (duplicated >= 0)
        {
            (void)shell_close(duplicated);
        }
    }
#endif
    return -1;
}
#endif

static bool is_space_char(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static esp_err_t copy_trimmed_span(const char *start, const char *end, char *out, size_t out_size)
{
    while (start < end && is_space_char(*start))
    {
        start++;
    }
    while (end > start && is_space_char(*(end - 1)))
    {
        end--;
    }

    size_t len = (size_t)(end - start);
    if (len + 1 > out_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return ESP_OK;
}

static esp_err_t parse_target_token(const char *start, const char *end, char *out, size_t out_size,
                                    const char **consumed_end)
{
    const char *p = start;
    while (p < end && is_space_char(*p))
    {
        p++;
    }
    if (p >= end)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char quote = '\0';
    if (*p == '"' || *p == '\'')
    {
        quote = *p++;
        const char *content_start = p;
        while (p < end && *p != quote)
        {
            p++;
        }
        if (p >= end || *p != quote)
        {
            return ESP_ERR_INVALID_ARG;
        }
        size_t len = (size_t)(p - content_start);
        if (len == 0 || len + 1 > out_size)
        {
            return len == 0 ? ESP_ERR_INVALID_ARG : ESP_ERR_INVALID_SIZE;
        }
        memcpy(out, content_start, len);
        out[len] = '\0';
        p++; /* closing quote */
    }
    else
    {
        const char *content_start = p;
        while (p < end && !is_space_char(*p))
        {
            char c = *p;
            if (c == '"' || c == '\'' || c == '>')
            {
                return ESP_ERR_INVALID_ARG;
            }
            p++;
        }
        size_t len = (size_t)(p - content_start);
        if (len == 0 || len + 1 > out_size)
        {
            return len == 0 ? ESP_ERR_INVALID_ARG : ESP_ERR_INVALID_SIZE;
        }
        memcpy(out, content_start, len);
        out[len] = '\0';
    }

    while (p < end && is_space_char(*p))
    {
        p++;
    }
    *consumed_end = p;
    return ESP_OK;
}

static bool segment_has_content(const char *start, const char *end)
{
    for (const char *p = start; p < end; p++)
    {
        if (!is_space_char(*p))
        {
            return true;
        }
    }
    return false;
}

esp_err_t shell_parse_pipeline(const char *line, shell_pipeline_info_t *out)
{
    if (line == NULL || out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    out->status = SHELL_PIPELINE_NONE;
    out->segment_count = 0;

    const char *segment_start = line;
    char quote = '\0';
    bool saw_pipe = false;
    size_t segments = 0;

    for (const char *p = line;; p++)
    {
        char c = *p;
        bool at_end = (c == '\0');
        bool is_pipe = false;

        if (!at_end)
        {
            if (quote != '\0')
            {
                if (c == quote)
                {
                    quote = '\0';
                }
            }
            else if (c == '"' || c == '\'')
            {
                quote = c;
            }
            else if (c == '|')
            {
                is_pipe = true;
            }
        }

        if (!is_pipe && !at_end)
        {
            continue;
        }

        if (!saw_pipe)
        {
            if (at_end)
            {
                return ESP_OK;
            }
            saw_pipe = true;
        }

        segments++;
        if (!segment_has_content(segment_start, p))
        {
            out->status = SHELL_PIPELINE_INVALID;
            out->segment_count = segments;
            return ESP_OK;
        }

        if (at_end)
        {
            out->status = SHELL_PIPELINE_UNSUPPORTED;
            out->segment_count = segments;
            return ESP_OK;
        }

        /* "||" is treated as invalid/unsupported in this slice (conservative). */
        if (*(p + 1) == '|')
        {
            out->status = SHELL_PIPELINE_INVALID;
            out->segment_count = segments + 1;
            return ESP_OK;
        }

        segment_start = p + 1;

        /* Detect trailing pipe (end of string after whitespace). */
        const char *rest = segment_start;
        while (*rest != '\0' && is_space_char(*rest))
        {
            rest++;
        }
        if (*rest == '\0')
        {
            out->status = SHELL_PIPELINE_INVALID;
            out->segment_count = segments + 1;
            return ESP_OK;
        }
    }
}

esp_err_t shell_parse_redirection(const char *line, shell_redirection_t *out)
{
    if (line == NULL || out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->type = SHELL_REDIRECT_NONE;

    const char *op = NULL;
    shell_redirect_type_t type = SHELL_REDIRECT_NONE;
    char quote = '\0';
    for (const char *p = line; *p != '\0'; p++)
    {
        char c = *p;
        if (quote != '\0')
        {
            if (c == quote)
            {
                quote = '\0';
            }
            continue;
        }
        if (c == '"' || c == '\'')
        {
            quote = c;
            continue;
        }
        if (c == '>')
        {
            op = p;
            if (*(p + 1) == '>')
            {
                if (*(p + 2) == '>')
                {
                    return ESP_ERR_INVALID_ARG;
                }
                type = SHELL_REDIRECT_APPEND;
            }
            else
            {
                type = SHELL_REDIRECT_TRUNCATE;
            }
            break;
        }
    }

    if (quote != '\0')
    {
        /* Unterminated quote in command portion: let existing parser/dispatch handle. */
        return ESP_OK;
    }

    if (op == NULL)
    {
        return ESP_OK;
    }

    const char *after_op = op + (type == SHELL_REDIRECT_APPEND ? 2 : 1);
    const char *line_end = line + strlen(line);

    esp_err_t err = copy_trimmed_span(line, op, out->command, sizeof(out->command));
    if (err != ESP_OK)
    {
        return err;
    }
    if (out->command[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *consumed_end = NULL;
    err = parse_target_token(after_op, line_end, out->target, sizeof(out->target), &consumed_end);
    if (err != ESP_OK)
    {
        return err;
    }
    if (consumed_end != line_end)
    {
        /* Extra tokens/operators after target (e.g. multiple redirections) are unsupported in this slice. */
        return ESP_ERR_INVALID_ARG;
    }

    out->type = type;
    return ESP_OK;
}

esp_err_t shell_redirect_begin(const shell_redirection_t *redir, shell_redirect_state_t *state)
{
    if (redir == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(state, 0, sizeof(*state));
    state->saved_stdout_fd = -1;
    state->saved_stdout = NULL;
    state->file = NULL;
    state->active = false;
    state->swapped_stdout = false;

    if (redir->type == SHELL_REDIRECT_NONE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char resolved_virtual[VFS_PATH_MAX];
    esp_err_t err = shell_resolve_relative(redir->target, resolved_virtual, sizeof(resolved_virtual));
    if (err != ESP_OK)
    {
        printf("redirection: resolve path failed for '%s': %s\n", redir->target, esp_err_to_name(err));
        return err;
    }

    if (!vfs_is_virtual_path(resolved_virtual))
    {
        printf("redirection: '%s' is not a virtual path\n", resolved_virtual);
        return ESP_ERR_INVALID_ARG;
    }
    if (vfs_exists(resolved_virtual) && vfs_is_directory(resolved_virtual))
    {
        printf("redirection: '%s' is a directory\n", resolved_virtual);
        return ESP_ERR_INVALID_ARG;
    }

    char real_path[VFS_PATH_MAX];
    err = vfs_resolve_path(resolved_virtual, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        printf("redirection: cannot resolve real path for '%s': %s\n", resolved_virtual, esp_err_to_name(err));
        return err;
    }

    const char *mode = (redir->type == SHELL_REDIRECT_APPEND) ? "a" : "w";
    FILE *target = fopen(real_path, mode);
    if (target == NULL)
    {
        printf("redirection: fopen('%s', \"%s\") failed\n", real_path, mode);
        return ESP_FAIL;
    }

    fflush(stdout);
    int stdout_fd = shell_fileno(stdout);
    int target_fd = shell_fileno(target);
    int saved_fd = -1;
    bool fd_redirect_ok = false;
    if (stdout_fd >= 0 && target_fd >= 0)
    {
        saved_fd = shell_dup(stdout_fd);
        if (saved_fd >= 0 && shell_dup2(target_fd, stdout_fd) == stdout_fd)
        {
            fd_redirect_ok = true;
        }
    }

    if (fd_redirect_ok)
    {
        state->active = true;
        state->saved_stdout_fd = saved_fd;
        state->file = target;
        state->swapped_stdout = false;
        state->saved_stdout = NULL;
        return ESP_OK;
    }

    if (saved_fd >= 0)
    {
        shell_close(saved_fd);
        saved_fd = -1;
    }

#if !defined(_WIN32)
    /* Fallback: swap stdout FILE* (works when commands write via stdio). */
    state->saved_stdout = stdout;
    stdout = target;
    state->active = true;
    state->saved_stdout_fd = -1;
    state->file = target;
    state->swapped_stdout = true;
    return ESP_OK;
#else
    fclose(target);
    printf("redirection: failed to duplicate/redirect stdout fd\n");
    return ESP_FAIL;
#endif
}

void shell_redirect_end(shell_redirect_state_t *state)
{
    if (state == NULL || !state->active)
    {
        return;
    }

#if !defined(_WIN32)
    if (state->swapped_stdout)
    {
        fflush(stdout);
        if (state->saved_stdout != NULL)
        {
            stdout = state->saved_stdout;
        }
        if (state->file != NULL)
        {
            fflush(state->file);
            fclose(state->file);
            state->file = NULL;
        }
        state->saved_stdout = NULL;
        state->active = false;
        state->swapped_stdout = false;
        return;
    }
#endif

    fflush(stdout);

    int stdout_fd = shell_fileno(stdout);
    if (state->saved_stdout_fd >= 0 && stdout_fd >= 0)
    {
        (void)shell_dup2(state->saved_stdout_fd, stdout_fd);
    }

    if (state->file != NULL)
    {
        fflush(state->file);
        fclose(state->file);
        state->file = NULL;
    }

    if (state->saved_stdout_fd >= 0)
    {
        shell_close(state->saved_stdout_fd);
        state->saved_stdout_fd = -1;
    }

    state->active = false;
    state->swapped_stdout = false;
    state->saved_stdout = NULL;
}
