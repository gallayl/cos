#pragma once

#include "esp_err.h"

#include <stddef.h>

void mock_berry_reset(void);
void mock_berry_set_init_result(esp_err_t result);
void mock_berry_set_eval_result(esp_err_t result);
void mock_berry_set_run_result(esp_err_t result);
void mock_berry_set_run_status(int status);
void mock_berry_set_close_result(esp_err_t result);
void mock_berry_set_gc_collect_result(esp_err_t result);
void mock_berry_set_gc_bytes(size_t before, size_t after);
void mock_berry_set_memory_info(bool initialized, size_t gc_managed_bytes);
const char *mock_berry_last_source(void);
const char *mock_berry_last_path(void);
const char *mock_berry_last_source_name(void);
int mock_berry_run_argc(void);
const char *mock_berry_run_arg(int index);

/* R21 compatibility aliases for older tests */
#define mock_berry_set_exec_result mock_berry_set_run_result
#define mock_berry_set_exec_status mock_berry_set_run_status
#define mock_berry_exec_argc mock_berry_run_argc
#define mock_berry_exec_arg mock_berry_run_arg
