#pragma once

#include "esp_err.h"
#include "filesystem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SHELL_REDIRECT_CMD_MAX 128

    typedef enum
    {
        SHELL_REDIRECT_NONE = 0,
        SHELL_REDIRECT_TRUNCATE,        /* >   stdout */
        SHELL_REDIRECT_APPEND,          /* >>  stdout */
        SHELL_REDIRECT_STDERR_TRUNCATE, /* 2>  stderr */
        SHELL_REDIRECT_STDERR_APPEND,   /* 2>> stderr */
        SHELL_REDIRECT_BOTH_TRUNCATE,   /* &>  stdout+stderr */
        SHELL_REDIRECT_BOTH_APPEND,     /* &>> stdout+stderr */
    } shell_redirect_type_t;

    typedef enum
    {
        SHELL_PIPELINE_NONE = 0,
        SHELL_PIPELINE_UNSUPPORTED,
        SHELL_PIPELINE_INVALID,
    } shell_pipeline_status_t;

    typedef struct
    {
        shell_pipeline_status_t status;
        size_t segment_count;
    } shell_pipeline_info_t;

    typedef enum
    {
        SHELL_FD_REDIRECT_NONE = 0,
        SHELL_FD_REDIRECT_UNSUPPORTED,
    } shell_fd_redirect_status_t;

    typedef struct
    {
        shell_fd_redirect_status_t status;
    } shell_fd_redirect_info_t;

    typedef struct
    {
        shell_redirect_type_t type;
        char command[SHELL_REDIRECT_CMD_MAX];
        char target[VFS_PATH_MAX];
    } shell_redirection_t;

    typedef struct
    {
        bool active;
        int saved_stdout_fd;
        int saved_stderr_fd;
        FILE *saved_stdout;
        FILE *saved_stderr;
        FILE *stdout_file;
        FILE *stderr_file;
        bool swapped_stdout;
        bool swapped_stderr;
    } shell_redirect_state_t;

    /**
     * Parse redirection syntax (>, >>, 2>, 2>>, &>, &>>) from a shell command line.
     *
     * - No unquoted redirection operator: returns ESP_OK and sets out->type to
     *   SHELL_REDIRECT_NONE (command/target are cleared).
     * - Supported redirection: returns ESP_OK and fills command, target, type.
     * - Malformed/unsupported syntax (missing command/target, multiple
     *   redirections, invalid operator): returns ESP_ERR_INVALID_ARG.
     * - Command or target exceeds buffer limits: returns ESP_ERR_INVALID_SIZE.
     *
     * Operators inside single or double quotes are ignored. The target may be a
     * quoted token; surrounding matching quotes are stripped. This function does
     * not perform file I/O or modify process streams.
     */
    esp_err_t shell_parse_redirection(const char *line, shell_redirection_t *out);

    /**
     * Detect unquoted pipe syntax ('|') in a shell command line.
     *
     * - No unquoted pipe: ESP_OK, status == SHELL_PIPELINE_NONE.
     * - Recognizable pipeline syntax (runtime unsupported in this slice): ESP_OK,
     *   status == SHELL_PIPELINE_UNSUPPORTED, segment_count >= 2.
     * - Malformed pipe syntax (leading/trailing/empty segment, '||' in this
     *   slice): ESP_OK, status == SHELL_PIPELINE_INVALID.
     *
     * Pipe characters inside single or double quotes are ignored.
     */
    esp_err_t shell_parse_pipeline(const char *line, shell_pipeline_info_t *out);

    /**
     * Detect unquoted stderr/FD/combined redirection syntax (2>, 2>>, &>, &>>).
     * Retained for compatibility; runtime-supported forms are parsed by
     * shell_parse_redirection().
     */
    esp_err_t shell_parse_fd_redirection(const char *line, shell_fd_redirect_info_t *out);

    /**
     * Redirect stdout/stderr to a VFS path according to the parsed redirection type.
     * Resolves relative targets from the current shell working directory.
     * Best-effort fd/stdio redirection: captures output written to redirected
     * stdio streams/fds; ESP log routing may differ by target/config.
     * On success, call shell_redirect_end() to restore streams.
     */
    esp_err_t shell_redirect_begin(const shell_redirection_t *redir, shell_redirect_state_t *state);

    /** Restore redirected streams after shell_redirect_begin(). Safe to call if inactive. */
    void shell_redirect_end(shell_redirect_state_t *state);

#ifdef __cplusplus
}
#endif
