#pragma once

#include "esp_err.h"

void mock_berry_reset(void);
void mock_berry_set_init_result(esp_err_t result);
void mock_berry_set_eval_result(esp_err_t result);
void mock_berry_set_run_result(esp_err_t result);
void mock_berry_set_exec_result(esp_err_t result);
void mock_berry_set_exec_status(int status);
void mock_berry_set_close_result(esp_err_t result);
const char *mock_berry_last_source(void);
const char *mock_berry_last_path(void);
const char *mock_berry_last_source_name(void);
int mock_berry_exec_argc(void);
const char *mock_berry_exec_arg(int index);
