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
#define SHELL_CLOSE _close
#define shell_dup _dup
#define shell_dup2 _dup2
#define SHELL_FILENO _fileno
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif
#else
#include <unistd.h>
#define SHELL_CLOSE close
#define SHELL_FILENO fileno
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

// NOLINTNEXTLINE(readability-identifier-naming) - bounded/intentional; preserve behavior
static int shell_dup(int fd)
{
    (void)fd;
#if defined(F_DUPFD_CLOEXEC)
    {
        int duplicated = fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (duplicated >= 0)
        {
            return duplicated;
        }
    }
#endif
#if defined(F_DUPFD)
    {
        int duplicated = fcntl(fd, F_DUPFD, 0);
        if (duplicated >= 0)
        {
            return duplicated;
        }
    }
#endif
    return -1;
}

// NOLINTNEXTLINE(readability-identifier-naming) - bounded/intentional; preserve behavior
static int shell_dup2(int oldfd, int newfd)
{
    if (oldfd == newfd)
    {
        return newfd;
    }
    if (SHELL_CLOSE(newfd) < 0)
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
            (void)SHELL_CLOSE(duplicated);
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
            (void)SHELL_CLOSE(duplicated);
        }
    }
#endif
    return -1;
}
#endif

static bool is_space_char(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\r') ||
           (c == '\n'); // NOLINT(readability-implicit-bool-conversion) - character comparisons in C bool return
}

static esp_err_t copy_trimmed_span(const char *start, const char *end, char *out, size_t out_size)
{
    while (start < end && is_space_char(*start)) // NOLINT(readability-implicit-bool-conversion) - C bool/int mix
    {
        start++;
    }
    while (end > start && is_space_char(*(end - 1))) // NOLINT(readability-implicit-bool-conversion) - C bool/int mix
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
    while (p < end && is_space_char(*p)) // NOLINT(readability-implicit-bool-conversion) - C bool/int mix
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

    while (p < end && is_space_char(*p)) // NOLINT(readability-implicit-bool-conversion) - C bool/int mix
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
        while (
            *rest != '\0' &&
            is_space_char(*rest)) // NOLINT(readability-implicit-bool-conversion) - bool/comparison mix in C condition
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

esp_err_t shell_parse_fd_redirection(const char *line, shell_fd_redirect_info_t *out)
{
    if (line == NULL || out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    out->status = SHELL_FD_REDIRECT_NONE;

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

        /* Detect 2>, 2>>, &>, &>> (recognized fd/combined redirection syntax). */
        if (c == '2' && *(p + 1) == '>')
        {
            out->status = SHELL_FD_REDIRECT_UNSUPPORTED; /* recognized syntax present */
            return ESP_OK;
        }
        if (c == '&' && *(p + 1) == '>' && (p == line || *(p - 1) != '<'))
        {
            out->status = SHELL_FD_REDIRECT_UNSUPPORTED;
            return ESP_OK;
        }
    }

    return ESP_OK;
}

static bool find_redirection_op(const char *line, const char **op_out, shell_redirect_type_t *type_out)
{
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

        if (c == '2' && *(p + 1) == '>')
        {
            if (*(p + 2) == '>')
            {
                if (*(p + 3) == '>')
                {
                    return false;
                }
                *op_out = p;
                *type_out = SHELL_REDIRECT_STDERR_APPEND;
            }
            else
            {
                *op_out = p;
                *type_out = SHELL_REDIRECT_STDERR_TRUNCATE;
            }
            return true;
        }

        if (c == '&' && *(p + 1) == '>' && (p == line || *(p - 1) != '<'))
        {
            if (*(p + 2) == '>')
            {
                if (*(p + 3) == '>')
                {
                    return false;
                }
                *op_out = p;
                *type_out = SHELL_REDIRECT_BOTH_APPEND;
            }
            else
            {
                *op_out = p;
                *type_out = SHELL_REDIRECT_BOTH_TRUNCATE;
            }
            return true;
        }

        if (c == '>')
        {
            /* Skip if part of a recognized fd/combined operator (handled above). */
            if (p != line && (*(p - 1) == '2' || *(p - 1) == '&'))
            {
                continue;
            }
            if (*(p + 1) == '>')
            {
                if (*(p + 2) == '>')
                {
                    return false;
                }
                *op_out = p;
                *type_out = SHELL_REDIRECT_APPEND;
            }
            else
            {
                *op_out = p;
                *type_out = SHELL_REDIRECT_TRUNCATE;
            }
            return true;
        }
    }

    if (quote != '\0')
    {
        /* Unterminated quote: let existing parser/dispatch handle. */
        *op_out = NULL;
        *type_out = SHELL_REDIRECT_NONE;
        return true;
    }

    *op_out = NULL;
    *type_out = SHELL_REDIRECT_NONE;
    return true;
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
    if (!find_redirection_op(line, &op, &type))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (op == NULL)
    {
        return ESP_OK;
    }

    size_t op_len = 1;
    switch (type)
    {
    case SHELL_REDIRECT_APPEND:
    case SHELL_REDIRECT_STDERR_TRUNCATE:
    case SHELL_REDIRECT_BOTH_TRUNCATE:
        op_len = 2;
        break;
    case SHELL_REDIRECT_STDERR_APPEND:
    case SHELL_REDIRECT_BOTH_APPEND:
        op_len = 3;
        break;
    default:
        op_len = 1;
        break;
    }

    const char *after_op = op + op_len;
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

static bool redirect_targets_stdout(shell_redirect_type_t type)
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion) - C logical OR result used as bool
    return (type == SHELL_REDIRECT_TRUNCATE) || (type == SHELL_REDIRECT_APPEND) ||
           (type == SHELL_REDIRECT_BOTH_TRUNCATE) || (type == SHELL_REDIRECT_BOTH_APPEND);
}

static bool redirect_targets_stderr(shell_redirect_type_t type)
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion) - C logical OR result used as bool
    return (type == SHELL_REDIRECT_STDERR_TRUNCATE) || (type == SHELL_REDIRECT_STDERR_APPEND) ||
           (type == SHELL_REDIRECT_BOTH_TRUNCATE) || (type == SHELL_REDIRECT_BOTH_APPEND);
}

static bool redirect_is_append(shell_redirect_type_t type)
{
    return (type == SHELL_REDIRECT_APPEND) || (type == SHELL_REDIRECT_STDERR_APPEND) ||
           (type == SHELL_REDIRECT_BOTH_APPEND); // NOLINT(readability-implicit-bool-conversion)
}

esp_err_t shell_redirect_begin(const shell_redirection_t *redir, shell_redirect_state_t *state)
{
    if (redir == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(state, 0, sizeof(*state));
    state->saved_stdout_fd = -1;
    state->saved_stderr_fd = -1;
    state->saved_stdout = NULL;
    state->saved_stderr = NULL;
    state->stdout_file = NULL;
    state->stderr_file = NULL;
    state->active = false;
    state->swapped_stdout = false;
    state->swapped_stderr = false;

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
    if (vfs_exists(resolved_virtual) &&
        vfs_is_directory(
            resolved_virtual)) // NOLINT(readability-implicit-bool-conversion) - bool/comparison mix in C condition
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

    const char *mode = redirect_is_append(redir->type)
                           ? "a"
                           : "w"; // NOLINT(readability-implicit-bool-conversion) - bool condition in C ternary
    const bool want_stdout = redirect_targets_stdout(redir->type);
    const bool want_stderr = redirect_targets_stderr(redir->type);

    if (!want_stdout && !want_stderr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *out_file = NULL;
    FILE *err_file = NULL;

    /* For &> / &>>, open a single file and share it. */
    FILE *target_file = fopen(real_path, mode);
    if (target_file == NULL)
    {
        printf("redirection: fopen('%s', \"%s\") failed\n", real_path, mode);
        return ESP_FAIL;
    }

    if (want_stdout)
    {
        out_file = target_file;
    }
    if (want_stderr)
    {
        err_file = target_file;
    }

    /* Best-effort redirection: try fd dup/dup2 per stream, else FILE* swap fallback (non-Windows). */
    if (want_stdout)
    {
        fflush(stdout);
        int fd = SHELL_FILENO(stdout);
        int target_fd = SHELL_FILENO(out_file);
        bool fd_ok = false;
        if (fd >= 0 && target_fd >= 0)
        {
            int saved = shell_dup(fd);
            if (saved >= 0 && shell_dup2(target_fd, fd) == fd)
            {
                state->saved_stdout_fd = saved;
                fd_ok = true;
            }
            else if (saved >= 0)
            {
                SHELL_CLOSE(saved);
            }
        }
        if (!fd_ok)
        {
#if !defined(_WIN32)
            state->saved_stdout = stdout;
            stdout = out_file;
            state->swapped_stdout = true;
#else
            fclose(target_file);
            printf("redirection: failed to redirect stdout\n");
            return ESP_FAIL;
#endif
        }
        state->stdout_file = out_file;
    }

    if (want_stderr)
    {
        fflush(stderr);
        int fd = SHELL_FILENO(stderr);
        int target_fd = SHELL_FILENO(err_file);
        bool fd_ok = false;
        if (fd >= 0 && target_fd >= 0)
        {
            int saved = shell_dup(fd);
            if (saved >= 0 && shell_dup2(target_fd, fd) == fd)
            {
                state->saved_stderr_fd = saved;
                fd_ok = true;
            }
            else if (saved >= 0)
            {
                SHELL_CLOSE(saved);
            }
        }
        if (!fd_ok)
        {
#if !defined(_WIN32)
            state->saved_stderr = stderr;
            stderr = err_file;
            state->swapped_stderr = true;
#else
            /* Roll back stdout redirection if any was applied. */
            if (want_stdout)
            {
                fflush(stdout);
                int sfd = SHELL_FILENO(stdout);
                if (state->saved_stdout_fd >= 0 && sfd >= 0)
                {
                    (void)shell_dup2(state->saved_stdout_fd, sfd);
                    SHELL_CLOSE(state->saved_stdout_fd);
                    state->saved_stdout_fd = -1;
                }
            }
            fclose(target_file);
            printf("redirection: failed to redirect stderr\n");
            return ESP_FAIL;
#endif
        }
        state->stderr_file = err_file;
    }

    state->active = true;
    return ESP_OK;
}

void shell_redirect_end(shell_redirect_state_t *state)
{
    if (state == NULL || !state->active)
    {
        return;
    }

#if !defined(_WIN32)
    if (state->swapped_stderr)
    {
        fflush(stderr);
        if (state->saved_stderr != NULL)
        {
            stderr = state->saved_stderr;
        }
        state->saved_stderr = NULL;
        state->swapped_stderr = false;
    }
    if (state->swapped_stdout)
    {
        fflush(stdout);
        if (state->saved_stdout != NULL)
        {
            stdout = state->saved_stdout;
        }
        state->saved_stdout = NULL;
        state->swapped_stdout = false;
    }
#endif

    fflush(stdout);
    fflush(stderr);

    int stdout_fd = SHELL_FILENO(stdout);
    if (state->saved_stdout_fd >= 0 && stdout_fd >= 0)
    {
        (void)shell_dup2(state->saved_stdout_fd, stdout_fd);
    }
    int stderr_fd = SHELL_FILENO(stderr);
    if (state->saved_stderr_fd >= 0 && stderr_fd >= 0)
    {
        (void)shell_dup2(state->saved_stderr_fd, stderr_fd);
    }

    /* Close target file(s) once (stdout_file and stderr_file may be same for &>). */
    if (state->stdout_file != NULL)
    {
        fflush(state->stdout_file);
        fclose(state->stdout_file);
    }
    if (state->stderr_file != NULL && state->stderr_file != state->stdout_file)
    {
        fflush(state->stderr_file);
        fclose(state->stderr_file);
    }

    if (state->saved_stdout_fd >= 0)
    {
        SHELL_CLOSE(state->saved_stdout_fd);
    }
    if (state->saved_stderr_fd >= 0)
    {
        SHELL_CLOSE(state->saved_stderr_fd);
    }

    memset(state, 0, sizeof(*state));
    state->saved_stdout_fd = -1;
    state->saved_stderr_fd = -1;
}
