#include "mock_berry.h"
#include "berry_runtime.h"

#include <string.h>

#define MOCK_BERRY_BUFFER_SIZE 512
#define MOCK_BERRY_MAX_ARGS 16

static esp_err_t s_init_result;
static esp_err_t s_eval_result;
static esp_err_t s_run_result;
static int s_run_status;
static int s_run_argc;
static char s_run_args[MOCK_BERRY_MAX_ARGS][MOCK_BERRY_BUFFER_SIZE];
static esp_err_t s_close_result;
static esp_err_t s_gc_collect_result;
static size_t s_gc_before;
static size_t s_gc_after;
static bool s_memory_initialized;
static size_t s_gc_managed_bytes;
static char s_last_source[MOCK_BERRY_BUFFER_SIZE];
static char s_last_path[MOCK_BERRY_BUFFER_SIZE];
static char s_last_source_name[MOCK_BERRY_BUFFER_SIZE];
static char s_piped_stdin[MOCK_BERRY_BUFFER_SIZE];
static bool s_piped_stdin_set;
static char s_last_piped_stdin_seen[MOCK_BERRY_BUFFER_SIZE];
static bool s_last_piped_stdin_set_seen;
static int s_run_call_count;
static esp_err_t s_run_result_seq[MOCK_BERRY_MAX_ARGS];
static int s_run_status_seq[MOCK_BERRY_MAX_ARGS];
static int s_run_seq_count;

void mock_berry_reset(void)
{
    s_init_result = ESP_OK;
    s_eval_result = ESP_OK;
    s_run_result = ESP_OK;
    s_run_status = 0;
    s_run_argc = 0;
    s_run_call_count = 0;
    s_run_seq_count = 0;
    memset(s_run_args, 0, sizeof(s_run_args));
    memset(s_run_result_seq, 0, sizeof(s_run_result_seq));
    memset(s_run_status_seq, 0, sizeof(s_run_status_seq));
    s_close_result = ESP_OK;
    s_gc_collect_result = ESP_OK;
    s_gc_before = 1024;
    s_gc_after = 512;
    s_memory_initialized = true;
    s_gc_managed_bytes = 512;
    s_last_source[0] = '\0';
    s_last_path[0] = '\0';
    s_last_source_name[0] = '\0';
    s_piped_stdin[0] = '\0';
    s_piped_stdin_set = false;
    s_last_piped_stdin_seen[0] = '\0';
    s_last_piped_stdin_set_seen = false;
}

void mock_berry_set_init_result(esp_err_t result)
{
    s_init_result = result;
}

void mock_berry_set_eval_result(esp_err_t result)
{
    s_eval_result = result;
}

void mock_berry_set_run_result(esp_err_t result)
{
    s_run_result = result;
}

void mock_berry_set_run_status(int status)
{
    s_run_status = status;
}

void mock_berry_set_run_results(const esp_err_t *results, const int *statuses, int count)
{
    s_run_seq_count = 0;
    if (results == NULL || statuses == NULL || count <= 0)
    {
        return;
    }
    if (count > MOCK_BERRY_MAX_ARGS)
    {
        count = MOCK_BERRY_MAX_ARGS;
    }
    for (int i = 0; i < count; i++)
    {
        s_run_result_seq[i] = results[i];
        s_run_status_seq[i] = statuses[i];
    }
    s_run_seq_count = count;
}

void mock_berry_set_close_result(esp_err_t result)
{
    s_close_result = result;
}

void mock_berry_set_gc_collect_result(esp_err_t result)
{
    s_gc_collect_result = result;
}

void mock_berry_set_gc_bytes(size_t before, size_t after)
{
    s_gc_before = before;
    s_gc_after = after;
}

void mock_berry_set_memory_info(bool initialized, size_t gc_managed_bytes)
{
    s_memory_initialized = initialized;
    s_gc_managed_bytes = gc_managed_bytes;
}

const char *mock_berry_last_source(void)
{
    return s_last_source;
}

const char *mock_berry_last_path(void)
{
    return s_last_path;
}

const char *mock_berry_last_source_name(void)
{
    return s_last_source_name;
}

int mock_berry_run_argc(void)
{
    return s_run_argc;
}

const char *mock_berry_run_arg(int index)
{
    return index >= 0 && index < s_run_argc ? s_run_args[index] : "";
}

esp_err_t berry_runtime_init(void)
{
    return s_init_result;
}

esp_err_t berry_runtime_eval(const char *source, const char *source_name)
{
    strncpy(s_last_source, source, sizeof(s_last_source) - 1);
    s_last_source[sizeof(s_last_source) - 1] = '\0';
    strncpy(s_last_source_name, source_name, sizeof(s_last_source_name) - 1);
    s_last_source_name[sizeof(s_last_source_name) - 1] = '\0';
    return s_eval_result;
}

esp_err_t berry_runtime_run_file(const char *virtual_path, int argc, char *const argv[], int *exit_status)
{
    strncpy(s_last_path, virtual_path != NULL ? virtual_path : "", sizeof(s_last_path) - 1);
    s_last_path[sizeof(s_last_path) - 1] = '\0';
    s_run_argc = argc < MOCK_BERRY_MAX_ARGS ? argc : MOCK_BERRY_MAX_ARGS;
    if (s_run_argc < 0)
    {
        s_run_argc = 0;
    }
    for (int index = 0; index < s_run_argc; ++index)
    {
        const char *arg = argv != NULL ? argv[index] : "";
        strncpy(s_run_args[index], arg != NULL ? arg : "", sizeof(s_run_args[index]) - 1);
        s_run_args[index][sizeof(s_run_args[index]) - 1] = '\0';
    }

    s_last_piped_stdin_set_seen = s_piped_stdin_set;
    if (s_piped_stdin_set)
    {
        strncpy(s_last_piped_stdin_seen, s_piped_stdin, sizeof(s_last_piped_stdin_seen) - 1);
        s_last_piped_stdin_seen[sizeof(s_last_piped_stdin_seen) - 1] = '\0';
    }
    else
    {
        s_last_piped_stdin_seen[0] = '\0';
    }

    esp_err_t result = s_run_result;
    int status = s_run_status;
    if (s_run_seq_count > 0)
    {
        int idx = s_run_call_count < s_run_seq_count ? s_run_call_count : (s_run_seq_count - 1);
        result = s_run_result_seq[idx];
        status = s_run_status_seq[idx];
    }
    s_run_call_count++;

    if (exit_status != NULL)
    {
        *exit_status = status;
    }
    return result;
}

esp_err_t berry_runtime_open_app(const char *virtual_path)
{
    int exit_status = 1;
    esp_err_t err = berry_runtime_run_file(virtual_path, 0, NULL, &exit_status);
    if (err != ESP_OK)
    {
        return err;
    }
    return exit_status == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t berry_runtime_close_app(void)
{
    return s_close_result;
}

void berry_runtime_set_console_input_provider(berry_console_read_line_fn_t read_line,
                                              berry_console_cancel_line_fn_t cancel_line, berry_console_read_key_fn_t read_key,
                                              berry_console_read_key_fn_t read_char)
{
    (void)read_line;
    (void)cancel_line;
    (void)read_key;
    (void)read_char;
}

esp_err_t berry_runtime_get_memory_info(berry_runtime_memory_info_t *info)
{
    if (info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(info, 0, sizeof(*info));
    info->initialized = s_memory_initialized;
    info->gc_managed_bytes = s_gc_managed_bytes;
    info->gc_step_rate = 200;
    info->heap_free = 200000;
    info->heap_total = 300000;
    info->heap_largest_block = 120000;
    info->heap_min_free = 180000;
    info->internal_free = 180000;
    info->internal_largest_block = 100000;
    info->internal_min_free = 160000;
    return ESP_OK;
}

esp_err_t berry_runtime_gc_collect(size_t *before_bytes, size_t *after_bytes)
{
    if (s_gc_collect_result == ESP_OK)
    {
        if (before_bytes != NULL)
        {
            *before_bytes = s_gc_before;
        }
        if (after_bytes != NULL)
        {
            *after_bytes = s_gc_after;
        }
        s_gc_managed_bytes = s_gc_after;
    }
    return s_gc_collect_result;
}

esp_err_t berry_runtime_set_gc_step_rate(int rate)
{
    (void)rate;
    return ESP_OK;
}

void mock_berry_set_piped_stdin(const char *data)
{
    berry_runtime_set_piped_stdin(data);
}

const char *mock_berry_piped_stdin(void)
{
    return s_piped_stdin;
}

bool mock_berry_piped_stdin_set(void)
{
    return s_piped_stdin_set;
}

const char *mock_berry_last_piped_stdin_seen(void)
{
    return s_last_piped_stdin_seen;
}

bool mock_berry_last_piped_stdin_set_seen(void)
{
    return s_last_piped_stdin_set_seen;
}

int mock_berry_run_call_count(void)
{
    return s_run_call_count;
}

void berry_runtime_set_piped_stdin(const char *data)
{
    if (data == NULL)
    {
        s_piped_stdin[0] = '\0';
        s_piped_stdin_set = false;
        return;
    }
    strncpy(s_piped_stdin, data, sizeof(s_piped_stdin) - 1);
    s_piped_stdin[sizeof(s_piped_stdin) - 1] = '\0';
    s_piped_stdin_set = true;
}

void berry_runtime_clear_piped_stdin(void)
{
    s_piped_stdin[0] = '\0';
    s_piped_stdin_set = false;
}
