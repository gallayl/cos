#include "mock_berry.h"
#include "berry_runtime.h"

#include <string.h>

#define MOCK_BERRY_BUFFER_SIZE 512
#define MOCK_BERRY_MAX_ARGS 16

static esp_err_t s_init_result;
static esp_err_t s_eval_result;
static esp_err_t s_run_result;
static esp_err_t s_exec_result;
static int s_exec_status;
static int s_exec_argc;
static char s_exec_args[MOCK_BERRY_MAX_ARGS][MOCK_BERRY_BUFFER_SIZE];
static esp_err_t s_close_result;
static esp_err_t s_gc_collect_result;
static size_t s_gc_before;
static size_t s_gc_after;
static bool s_memory_initialized;
static size_t s_gc_managed_bytes;
static char s_last_source[MOCK_BERRY_BUFFER_SIZE];
static char s_last_path[MOCK_BERRY_BUFFER_SIZE];
static char s_last_source_name[MOCK_BERRY_BUFFER_SIZE];

void mock_berry_reset(void)
{
    s_init_result = ESP_OK;
    s_eval_result = ESP_OK;
    s_run_result = ESP_OK;
    s_exec_result = ESP_OK;
    s_exec_status = 0;
    s_exec_argc = 0;
    memset(s_exec_args, 0, sizeof(s_exec_args));
    s_close_result = ESP_OK;
    s_gc_collect_result = ESP_OK;
    s_gc_before = 1024;
    s_gc_after = 512;
    s_memory_initialized = true;
    s_gc_managed_bytes = 512;
    s_last_source[0] = '\0';
    s_last_path[0] = '\0';
    s_last_source_name[0] = '\0';
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

void mock_berry_set_exec_result(esp_err_t result)
{
    s_exec_result = result;
}

void mock_berry_set_exec_status(int status)
{
    s_exec_status = status;
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

int mock_berry_exec_argc(void)
{
    return s_exec_argc;
}

const char *mock_berry_exec_arg(int index)
{
    return index >= 0 && index < s_exec_argc ? s_exec_args[index] : "";
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

esp_err_t berry_runtime_run_file(const char *virtual_path)
{
    strncpy(s_last_path, virtual_path, sizeof(s_last_path) - 1);
    s_last_path[sizeof(s_last_path) - 1] = '\0';
    return s_run_result;
}

esp_err_t berry_runtime_exec_file(const char *virtual_path, int argc, char *const argv[], int *exit_status)
{
    strncpy(s_last_path, virtual_path, sizeof(s_last_path) - 1);
    s_last_path[sizeof(s_last_path) - 1] = '\0';
    s_exec_argc = argc < MOCK_BERRY_MAX_ARGS ? argc : MOCK_BERRY_MAX_ARGS;
    for (int index = 0; index < s_exec_argc; ++index)
    {
        strncpy(s_exec_args[index], argv[index], sizeof(s_exec_args[index]) - 1);
        s_exec_args[index][sizeof(s_exec_args[index]) - 1] = '\0';
    }
    if (s_exec_result == ESP_OK)
    {
        *exit_status = s_exec_status;
    }
    return s_exec_result;
}

esp_err_t berry_runtime_close_app(void)
{
    return s_close_result;
}

void berry_runtime_set_console_input_provider(berry_console_read_line_fn read_line,
                                              berry_console_cancel_line_fn cancel_line, berry_console_read_key_fn read_key,
                                              berry_console_read_key_fn read_char)
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
