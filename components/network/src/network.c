#include "network.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "wifi.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define PING_TASK_STACK 4096
#define PING_TASK_PRIORITY 4

typedef struct
{
    SemaphoreHandle_t completed;
    bool replied;
    uint32_t rtt_ms;
} ping_context_t;

typedef struct
{
    uint32_t request_id;
    char host[NETWORK_HOST_MAX + 1];
    int64_t deadline_us;
} ping_request_t;

static QueueHandle_t s_ping_queue;
static SemaphoreHandle_t s_ping_mutex;
static SemaphoreHandle_t s_ping_completed_sem;
static TaskHandle_t s_ping_task;
static bool s_ping_active;
static bool s_ping_abandoned;
static bool s_ping_completed;
static uint32_t s_next_request_id;
static uint32_t s_active_request_id;
static uint32_t s_completed_request_id;
static esp_err_t s_completed_error;
static network_ping_result_t s_completed_result;

static TickType_t time_until_deadline_ticks(int64_t deadline_us)
{
    const int64_t remaining_us = deadline_us - esp_timer_get_time();
    if (remaining_us <= 0)
    {
        return 0;
    }

    const uint32_t remaining_ms = (uint32_t)((remaining_us + 999) / 1000);
    return pdMS_TO_TICKS(remaining_ms);
}

static bool valid_hostname(const char *host)
{
    if (host == NULL || host[0] == '\0' || strnlen(host, NETWORK_HOST_MAX + 1) > NETWORK_HOST_MAX)
    {
        return false;
    }

    size_t label_length = 0;
    bool previous_hyphen = false;
    for (const unsigned char *character = (const unsigned char *)host; *character != '\0'; ++character)
    {
        if (*character == '.')
        {
            if (label_length == 0 || previous_hyphen)
            {
                return false;
            }
            label_length = 0;
            previous_hyphen = false;
            continue;
        }
        if (!isalnum(*character) && *character != '-')
        {
            return false;
        }
        if (label_length == 0 && *character == '-')
        {
            return false;
        }
        previous_hyphen = *character == '-';
        if (++label_length > 63)
        {
            return false;
        }
    }
    return label_length > 0 && !previous_hyphen;
}

static void ping_on_success(esp_ping_handle_t handle, void *argument)
{
    ping_context_t *context = (ping_context_t *)argument;
    uint32_t rtt_ms = 0;
    if (esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &rtt_ms, sizeof(rtt_ms)) == ESP_OK)
    {
        context->rtt_ms = rtt_ms;
        context->replied = true;
    }
}

static void ping_on_end(esp_ping_handle_t handle, void *argument)
{
    (void)handle;
    ping_context_t *context = (ping_context_t *)argument;
    xSemaphoreGive(context->completed);
}

static esp_err_t perform_ping(const char *host, int64_t deadline_us, network_ping_result_t *result)
{
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *addresses = NULL;
    const int lookup_result = getaddrinfo(host, NULL, &hints, &addresses);
    if (lookup_result != 0 || addresses == NULL || addresses->ai_addr == NULL || addresses->ai_family != AF_INET ||
        addresses->ai_addrlen < sizeof(struct sockaddr_in))
    {
        if (addresses != NULL)
        {
            freeaddrinfo(addresses);
        }
        return ESP_ERR_NOT_FOUND;
    }

    const struct sockaddr_in *socket_address = (const struct sockaddr_in *)addresses->ai_addr;
    ip_addr_t target = {0};
    target.type = IPADDR_TYPE_V4;
    target.u_addr.ip4.addr = socket_address->sin_addr.s_addr;
    if (inet_ntop(AF_INET, &socket_address->sin_addr, result->address, sizeof(result->address)) == NULL)
    {
        freeaddrinfo(addresses);
        return ESP_FAIL;
    }
    freeaddrinfo(addresses);

    const int64_t remaining_us = deadline_us - esp_timer_get_time();
    if (remaining_us <= 0)
    {
        return ESP_ERR_TIMEOUT;
    }
    const uint32_t timeout_ms = (uint32_t)(remaining_us / 1000);
    if (timeout_ms == 0)
    {
        return ESP_ERR_TIMEOUT;
    }

    ping_context_t context = {
        .completed = xSemaphoreCreateBinary(),
        .replied = false,
        .rtt_ms = 0,
    };
    if (context.completed == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = 1;
    config.interval_ms = 1000;
    config.timeout_ms = timeout_ms;
    config.target_addr = target;
    const esp_ping_callbacks_t callbacks = {
        .cb_args = &context,
        .on_ping_success = ping_on_success,
        .on_ping_timeout = NULL,
        .on_ping_end = ping_on_end,
    };
    esp_ping_handle_t session = NULL;
    esp_err_t err = esp_ping_new_session(&config, &callbacks, &session);
    if (err == ESP_OK)
    {
        err = esp_ping_start(session);
    }
    if (err == ESP_OK && xSemaphoreTake(context.completed, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        err = ESP_ERR_TIMEOUT;
        (void)esp_ping_stop(session);
    }
    if (session != NULL)
    {
        esp_err_t delete_err = esp_ping_delete_session(session);
        if (err == ESP_OK && delete_err != ESP_OK)
        {
            err = delete_err;
        }
    }
    vSemaphoreDelete(context.completed);
    if (err == ESP_OK)
    {
        if (context.replied)
        {
            result->rtt_ms = context.rtt_ms;
        }
        else
        {
            err = ESP_ERR_TIMEOUT;
        }
    }
    return err;
}

static void ping_worker(void *argument)
{
    (void)argument;
    ping_request_t request;
    for (;;)
    {
        if (xQueueReceive(s_ping_queue, &request, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        network_ping_result_t result = {0};
        const int64_t now_us = esp_timer_get_time();
        esp_err_t err = ESP_ERR_TIMEOUT;
        if (now_us < request.deadline_us)
        {
            err = perform_ping(request.host, request.deadline_us, &result);
            if (esp_timer_get_time() >= request.deadline_us)
            {
                err = ESP_ERR_TIMEOUT;
            }
        }

        xSemaphoreTake(s_ping_mutex, portMAX_DELAY);
        if (s_ping_active && s_active_request_id == request.request_id)
        {
            s_ping_active = false;
            if (!s_ping_abandoned)
            {
                s_completed_request_id = request.request_id;
                s_completed_error = err;
                s_completed_result = result;
                s_ping_completed = true;
                xSemaphoreGive(s_ping_completed_sem);
            }
        }
        xSemaphoreGive(s_ping_mutex);
    }
}

esp_err_t network_init(void)
{
    if (s_ping_queue != NULL && s_ping_mutex != NULL && s_ping_completed_sem != NULL && s_ping_task != NULL)
    {
        return ESP_OK;
    }

    SemaphoreHandle_t ping_mutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t completed_sem = xSemaphoreCreateBinary();
    QueueHandle_t ping_queue = xQueueCreate(1, sizeof(ping_request_t));
    if (ping_mutex == NULL || completed_sem == NULL || ping_queue == NULL)
    {
        if (ping_mutex != NULL)
        {
            vSemaphoreDelete(ping_mutex);
        }
        if (completed_sem != NULL)
        {
            vSemaphoreDelete(completed_sem);
        }
        if (ping_queue != NULL)
        {
            vQueueDelete(ping_queue);
        }
        return ESP_ERR_NO_MEM;
    }

    s_ping_mutex = ping_mutex;
    s_ping_completed_sem = completed_sem;
    s_ping_queue = ping_queue;
    if (xTaskCreate(ping_worker, "net_ping", PING_TASK_STACK, NULL, PING_TASK_PRIORITY, &s_ping_task) != pdPASS)
    {
        s_ping_mutex = NULL;
        s_ping_completed_sem = NULL;
        s_ping_queue = NULL;
        vSemaphoreDelete(ping_mutex);
        vSemaphoreDelete(completed_sem);
        vQueueDelete(ping_queue);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t network_ping(const char *host, uint32_t timeout_ms, network_ping_result_t *result)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(result, 0, sizeof(*result));
    if (!valid_hostname(host) || timeout_ms < NETWORK_PING_TIMEOUT_MIN_MS || timeout_ms > NETWORK_PING_TIMEOUT_MAX_MS)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const int64_t deadline_us = esp_timer_get_time() + ((int64_t)timeout_ms * 1000);
    if (!wifi_is_connected())
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ping_queue == NULL || s_ping_mutex == NULL || s_ping_completed_sem == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_ping_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    if (s_ping_active || s_ping_completed)
    {
        xSemaphoreGive(s_ping_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    while (xSemaphoreTake(s_ping_completed_sem, 0) == pdTRUE)
    {
    }
    ping_request_t request = {
        .request_id = ++s_next_request_id,
        .deadline_us = deadline_us,
    };
    memcpy(request.host, host, strlen(host) + 1);
    s_ping_active = true;
    s_ping_abandoned = false;
    s_active_request_id = request.request_id;
    xSemaphoreGive(s_ping_mutex);

    if (xQueueSend(s_ping_queue, &request, 0) != pdTRUE)
    {
        xSemaphoreTake(s_ping_mutex, portMAX_DELAY);
        s_ping_active = false;
        xSemaphoreGive(s_ping_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    const TickType_t remaining_ticks = time_until_deadline_ticks(deadline_us);
    if (remaining_ticks > 0 && xSemaphoreTake(s_ping_completed_sem, remaining_ticks) == pdTRUE)
    {
        xSemaphoreTake(s_ping_mutex, portMAX_DELAY);
        const esp_err_t err = s_ping_completed && s_completed_request_id == request.request_id ? s_completed_error
                                                                                               : ESP_ERR_INVALID_STATE;
        if (s_ping_completed && s_completed_request_id == request.request_id)
        {
            *result = s_completed_result;
            s_ping_completed = false;
        }
        xSemaphoreGive(s_ping_mutex);
        return err;
    }

    xSemaphoreTake(s_ping_mutex, portMAX_DELAY);
    esp_err_t err = ESP_ERR_TIMEOUT;
    if (s_ping_completed && s_completed_request_id == request.request_id)
    {
        err = s_completed_error;
        *result = s_completed_result;
        s_ping_completed = false;
        (void)xSemaphoreTake(s_ping_completed_sem, 0);
    }
    else if (s_ping_active && s_active_request_id == request.request_id)
    {
        s_ping_abandoned = true;
    }
    xSemaphoreGive(s_ping_mutex);
    return err;
}
