#include "mock_network.h"

#include <string.h>

static esp_err_t s_ping_error;
static esp_err_t s_resolve_error;
static network_ping_result_t s_ping_result;
static network_ping_result_t s_resolve_result;
static char s_last_host[NETWORK_HOST_MAX + 1];
static uint32_t s_last_timeout;

void mock_network_reset(void)
{
    s_ping_error = ESP_OK;
    s_resolve_error = ESP_OK;
    memset(&s_ping_result, 0, sizeof(s_ping_result));
    memset(&s_resolve_result, 0, sizeof(s_resolve_result));
    strcpy(s_ping_result.address, "192.0.2.1");
    strcpy(s_resolve_result.address, "192.0.2.2");
    s_last_host[0] = '\0';
    s_last_timeout = 0;
}

void mock_network_set_ping_result(esp_err_t err, const network_ping_result_t *result)
{
    s_ping_error = err;
    if (result != NULL)
    {
        s_ping_result = *result;
    }
}

void mock_network_set_resolve_result(esp_err_t err, const network_ping_result_t *result)
{
    s_resolve_error = err;
    if (result != NULL)
    {
        s_resolve_result = *result;
    }
}

const char *mock_network_last_host(void)
{
    return s_last_host;
}

uint32_t mock_network_last_timeout(void)
{
    return s_last_timeout;
}

esp_err_t network_ping(const char *host, uint32_t timeout_ms, network_ping_result_t *result)
{
    strncpy(s_last_host, host, sizeof(s_last_host) - 1);
    s_last_host[sizeof(s_last_host) - 1] = '\0';
    s_last_timeout = timeout_ms;
    if (result != NULL)
    {
        *result = s_ping_result;
    }
    return s_ping_error;
}

esp_err_t network_resolve(const char *host, uint32_t timeout_ms, network_ping_result_t *result)
{
    strncpy(s_last_host, host, sizeof(s_last_host) - 1);
    s_last_host[sizeof(s_last_host) - 1] = '\0';
    s_last_timeout = timeout_ms;
    if (result != NULL)
    {
        *result = s_resolve_result;
    }
    return s_resolve_error;
}
