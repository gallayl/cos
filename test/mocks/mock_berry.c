#include "mock_berry.h"
#include "berry_runtime.h"

#include <string.h>

#define MOCK_BERRY_BUFFER_SIZE 512

static esp_err_t s_init_result;
static esp_err_t s_eval_result;
static esp_err_t s_run_result;
static esp_err_t s_close_result;
static char s_last_source[MOCK_BERRY_BUFFER_SIZE];
static char s_last_path[MOCK_BERRY_BUFFER_SIZE];
static char s_last_source_name[MOCK_BERRY_BUFFER_SIZE];

void mock_berry_reset(void)
{
    s_init_result = ESP_OK;
    s_eval_result = ESP_OK;
    s_run_result = ESP_OK;
    s_close_result = ESP_OK;
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

void mock_berry_set_close_result(esp_err_t result)
{
    s_close_result = result;
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

esp_err_t berry_runtime_close_app(void)
{
    return s_close_result;
}
