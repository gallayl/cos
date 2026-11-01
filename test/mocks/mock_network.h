#pragma once

#include "esp_err.h"
#include "network.h"

void mock_network_reset(void);
void mock_network_set_ping_result(esp_err_t err, const network_ping_result_t *result);
void mock_network_set_resolve_result(esp_err_t err, const network_ping_result_t *result);
const char *mock_network_last_host(void);
uint32_t mock_network_last_timeout(void);
