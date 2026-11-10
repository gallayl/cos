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
        SHELL_REDIRECT_TRUNCATE, /* >  */
        SHELL_REDIRECT_APPEND,   /* >> */
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
        FILE *saved_stdout;
        FILE *file;
        bool swapped_stdout;
    } shell_redirect_state_t;

    /**
     * Parse stdout redirection syntax (> / >>) from a shell command line.
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
     * Redirect stdout to a VFS path according to the parsed redirection type.
     * Resolves relative targets from the current shell working directory.
     * On success, call shell_redirect_end() to restore stdout.
     */
    esp_err_t shell_redirect_begin(const shell_redirection_t *redir, shell_redirect_state_t *state);

    /** Restore stdout after shell_redirect_begin(). Safe to call if inactive. */
    void shell_redirect_end(shell_redirect_state_t *state);

#ifdef __cplusplus
}
#endif
