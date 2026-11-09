#include "network.h"

#include "wifi.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>

// R8/P4: ESP-IDF xTaskCreate takes stack depth in bytes on ESP32 targets used
// here (log showed cfg=12288 unit=1). First hardware sample used ~1796 bytes
// (HWM 10492). 6144 gives ~4KB margin while freeing 6KB vs 12288; remeasure
// GET/POST/HTTPS/max-response/timeout/error before reducing further.
#define HTTP_TASK_STACK 6144
#define HTTP_TASK_PRIORITY 4

static const char *TAG = "net_http";

// Single-flight HTTP worker: one request at a time. Request/response buffers and
// s_response are shared static state owned by the worker/request path while
// active. On caller timeout the request is marked abandoned; the worker/event
// handler stops mutating public response state and the caller receives a stable
// empty snapshot, but the worker may continue until esp_http_client_perform
// returns. A new request is rejected (ESP_ERR_INVALID_STATE) until the previous
// worker invocation completes. s_http_mutex guards the handoff/state flags.

typedef struct
{
    uint32_t request_id;
    esp_http_client_method_t method;
    uint32_t timeout_ms;
    int64_t deadline_us;
    size_t body_len;
    size_t header_count;
} http_message_t;

typedef struct
{
    uint32_t request_id;
    int64_t deadline_us;
    esp_err_t error;
    size_t header_count;
    size_t body_len;
} http_event_ctx_t;

static QueueHandle_t s_http_queue;
static SemaphoreHandle_t s_http_mutex;
static SemaphoreHandle_t s_http_done;
static TaskHandle_t s_http_task;
static bool s_http_active;
static bool s_http_abandoned;
static bool s_http_completed;
static uint32_t s_next_request_id;
static uint32_t s_active_request_id;
static uint32_t s_completed_request_id;
static esp_err_t s_completed_error;

static char s_req_url[NETWORK_URL_MAX + 1];
static char s_req_body[NETWORK_HTTP_BODY_MAX + 1];
static network_http_header_t s_req_headers[NETWORK_HTTP_HEADER_MAX];
static char s_resp_body[NETWORK_HTTP_BODY_MAX + 1];
static const char s_empty_body[] = "";
static network_http_response_t s_response;

static network_http_worker_stack_info_t s_http_stack_info;

static bool http_request_abandoned(uint32_t request_id)
{
    bool abandoned = false;
    if (s_http_mutex != NULL && xSemaphoreTake(s_http_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        abandoned = s_http_abandoned && s_http_active && s_active_request_id == request_id;
        xSemaphoreGive(s_http_mutex);
    }
    return abandoned;
}

static void http_clear_public_response(void)
{
    s_response.status = 0;
    s_response.header_count = 0;
    s_response.body_len = 0;
    s_response.body = s_empty_body;
}

static const char *http_method_name(esp_http_client_method_t method)
{
    switch (method)
    {
    case HTTP_METHOD_POST:
        return "POST";
    case HTTP_METHOD_PUT:
        return "PUT";
    case HTTP_METHOD_PATCH:
        return "PATCH";
    case HTTP_METHOD_DELETE:
        return "DELETE";
    case HTTP_METHOD_HEAD:
        return "HEAD";
    case HTTP_METHOD_GET:
    default:
        return "GET";
    }
}

static void log_http_worker_stack(const char *method, bool https, int status, esp_err_t err)
{
    const UBaseType_t hwm_words = uxTaskGetStackHighWaterMark(NULL);
    const uint32_t stack_unit = (uint32_t)sizeof(StackType_t);
    const uint32_t hwm_bytes = (uint32_t)hwm_words * stack_unit;
    const uint32_t configured = (uint32_t)HTTP_TASK_STACK;
    const uint32_t used_bytes = configured > hwm_bytes ? configured - hwm_bytes : 0;

    ESP_LOGI(TAG,
             "R8/P4 stack: cfg=%u unit=%u hwm_words=%u hwm_bytes=%u used~=%u min_free=%u method=%s scheme=%s status=%d "
             "err=%s",
             (unsigned)configured, (unsigned)stack_unit, (unsigned)hwm_words, (unsigned)hwm_bytes, (unsigned)used_bytes,
             (unsigned)hwm_bytes, method != NULL ? method : "?", https ? "https" : "http", status,
             esp_err_to_name(err));

    network_http_worker_stack_info_t sample = {0};
    sample.available = true;
    sample.configured_stack = configured;
    sample.stack_unit_bytes = stack_unit;
    sample.hwm_words = (uint32_t)hwm_words;
    sample.hwm_bytes = hwm_bytes;
    sample.used_bytes = used_bytes;
    sample.https = https;
    sample.last_status = status;
    sample.last_error = err;
    strncpy(sample.last_method, method != NULL ? method : "?", sizeof(sample.last_method) - 1);
    sample.last_method[sizeof(sample.last_method) - 1] = '\0';

    if (s_http_mutex != NULL && xSemaphoreTake(s_http_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_http_stack_info = sample;
        xSemaphoreGive(s_http_mutex);
    }
    else
    {
        s_http_stack_info = sample;
    }
}

esp_err_t network_http_get_worker_stack_info(network_http_worker_stack_info_t *info)
{
    if (info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(info, 0, sizeof(*info));
    if (s_http_mutex != NULL && xSemaphoreTake(s_http_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        *info = s_http_stack_info;
        xSemaphoreGive(s_http_mutex);
    }
    else
    {
        *info = s_http_stack_info;
    }
    if (!info->available)
    {
        info->configured_stack = (uint32_t)HTTP_TASK_STACK;
        info->stack_unit_bytes = (uint32_t)sizeof(StackType_t);
    }
    return ESP_OK;
}

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

/** Copy source to destination as lowercase ASCII, NUL-terminating within destination_size. */
static void copy_lower(char *destination, size_t destination_size, const char *source)
{
    size_t index = 0;
    while (source[index] != '\0' && index + 1 < destination_size)
    {
        char character = source[index];
        if (character >= 'A' && character <= 'Z')
        {
            character = (char)(character - 'A' + 'a');
        }
        destination[index] = character;
        ++index;
    }
    destination[index] = '\0';
}

static esp_http_client_method_t method_from_name(const char *method)
{
    if (strcmp(method, "POST") == 0)
    {
        return HTTP_METHOD_POST;
    }
    if (strcmp(method, "PUT") == 0)
    {
        return HTTP_METHOD_PUT;
    }
    if (strcmp(method, "PATCH") == 0)
    {
        return HTTP_METHOD_PATCH;
    }
    if (strcmp(method, "DELETE") == 0)
    {
        return HTTP_METHOD_DELETE;
    }
    if (strcmp(method, "HEAD") == 0)
    {
        return HTTP_METHOD_HEAD;
    }
    return HTTP_METHOD_GET;
}

/** esp_http_client event handler. Copies headers/body into shared buffers, enforces
 *  size/deadline limits, and aborts the transfer when the caller has timed out. */
static esp_err_t http_event(esp_http_client_event_t *event)
{
    http_event_ctx_t *context = (http_event_ctx_t *)event->user_data;
    if (context->error != ESP_OK)
    {
        return context->error;
    }
    if (http_request_abandoned(context->request_id))
    {
        context->error = ESP_ERR_TIMEOUT;
        return ESP_ERR_TIMEOUT;
    }
    if (esp_timer_get_time() >= context->deadline_us)
    {
        context->error = ESP_ERR_TIMEOUT;
        return ESP_ERR_TIMEOUT;
    }

    switch (event->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
        context->header_count = 0;
        context->body_len = 0;
        s_resp_body[0] = '\0';
        break;
    case HTTP_EVENT_ON_HEADER:
        if (http_request_abandoned(context->request_id))
        {
            context->error = ESP_ERR_TIMEOUT;
            return ESP_ERR_TIMEOUT;
        }
        if (event->header_key != NULL && event->header_value != NULL &&
            context->header_count < NETWORK_HTTP_RESPONSE_HEADER_MAX &&
            strlen(event->header_key) <= NETWORK_HTTP_HEADER_NAME_MAX &&
            strlen(event->header_value) <= NETWORK_HTTP_HEADER_VALUE_MAX)
        {
            network_http_header_t *header = &s_response.headers[context->header_count];
            copy_lower(header->name, sizeof(header->name), event->header_key);
            memcpy(header->value, event->header_value, strlen(event->header_value) + 1);
            ++context->header_count;
        }
        break;
    case HTTP_EVENT_ON_DATA:
        if (event->data_len < 0 || context->body_len + (size_t)event->data_len > NETWORK_HTTP_BODY_MAX)
        {
            context->error = ESP_ERR_INVALID_SIZE;
            context->body_len = 0;
            s_resp_body[0] = '\0';
            return ESP_ERR_INVALID_SIZE;
        }
        if (http_request_abandoned(context->request_id))
        {
            context->error = ESP_ERR_TIMEOUT;
            return ESP_ERR_TIMEOUT;
        }
        if (event->data_len > 0 && event->data != NULL)
        {
            memcpy(s_resp_body + context->body_len, event->data, (size_t)event->data_len);
            context->body_len += (size_t)event->data_len;
            s_resp_body[context->body_len] = '\0';
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static esp_err_t perform_request(const http_message_t *message)
{
    http_event_ctx_t context = {
        .request_id = message->request_id,
        .deadline_us = message->deadline_us,
        .error = ESP_OK,
        .header_count = 0,
        .body_len = 0,
    };
    s_resp_body[0] = '\0';
    http_clear_public_response();
    s_response.body = s_resp_body;

    if (http_request_abandoned(message->request_id))
    {
        http_clear_public_response();
        return ESP_ERR_TIMEOUT;
    }
    if (esp_timer_get_time() >= message->deadline_us)
    {
        http_clear_public_response();
        return ESP_ERR_TIMEOUT;
    }

    esp_http_client_config_t config = {
        .url = s_req_url,
        .timeout_ms = (int)message->timeout_ms,
        .event_handler = http_event,
        .user_data = &context,
        // Smaller esp_http_client I/O buffers reduce peak heap during TLS setup.
        .buffer_size = 512,
        .buffer_size_tx = 256,
        .max_redirection_count = 3,
    };
    if (strncmp(s_req_url, "https://", 8) == 0)
    {
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_http_client_set_method(client, message->method);
    if (err == ESP_OK)
    {
        err = esp_http_client_set_header(client, "User-Agent", "COS");
    }
    for (size_t index = 0; err == ESP_OK && index < message->header_count; ++index)
    {
        err = esp_http_client_set_header(client, s_req_headers[index].name, s_req_headers[index].value);
    }
    if (err == ESP_OK && message->body_len > 0)
    {
        err = esp_http_client_set_post_field(client, s_req_body, (int)message->body_len);
    }
    if (err == ESP_OK)
    {
        err = esp_http_client_perform(client);
    }
    if (context.error != ESP_OK)
    {
        err = context.error;
    }
    if (http_request_abandoned(message->request_id))
    {
        esp_http_client_cleanup(client);
        http_clear_public_response();
        return ESP_ERR_TIMEOUT;
    }

    if (err == ESP_OK)
    {
        const int status = esp_http_client_get_status_code(client);
        if (status <= 0)
        {
            err = ESP_FAIL;
        }
        else
        {
            s_response.status = status;
            s_response.header_count = context.header_count;
            s_response.body_len = context.body_len;
            s_response.body = s_resp_body;
        }
    }
    if (err != ESP_OK)
    {
        http_clear_public_response();
        s_resp_body[0] = '\0';
        s_response.body = s_resp_body;
    }
    esp_http_client_cleanup(client);
    return err;
}

static void http_worker(void *argument)
{
    (void)argument;
    http_message_t message;
    for (;;)
    {
        if (xQueueReceive(s_http_queue, &message, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        const esp_err_t err = perform_request(&message);
        const int status = s_response.status;
        const bool https = strncmp(s_req_url, "https://", 8) == 0;
        log_http_worker_stack(http_method_name(message.method), https, status, err);
        xSemaphoreTake(s_http_mutex, portMAX_DELAY);
        if (s_http_active && s_active_request_id == message.request_id)
        {
            s_http_active = false;
            if (!s_http_abandoned)
            {
                s_completed_request_id = message.request_id;
                s_completed_error = err;
                s_http_completed = true;
                xSemaphoreGive(s_http_done);
            }
        }
        xSemaphoreGive(s_http_mutex);
    }
}

static esp_err_t ensure_http_worker(void)
{
    if (s_http_queue != NULL && s_http_mutex != NULL && s_http_done != NULL && s_http_task != NULL)
    {
        return ESP_OK;
    }

    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t done = xSemaphoreCreateBinary();
    QueueHandle_t queue = xQueueCreate(1, sizeof(http_message_t));
    if (mutex == NULL || done == NULL || queue == NULL)
    {
        if (mutex != NULL)
        {
            vSemaphoreDelete(mutex);
        }
        if (done != NULL)
        {
            vSemaphoreDelete(done);
        }
        if (queue != NULL)
        {
            vQueueDelete(queue);
        }
        return ESP_ERR_NO_MEM;
    }

    s_http_mutex = mutex;
    s_http_done = done;
    s_http_queue = queue;
    if (xTaskCreate(http_worker, "net_http", HTTP_TASK_STACK, NULL, HTTP_TASK_PRIORITY, &s_http_task) != pdPASS)
    {
        s_http_mutex = NULL;
        s_http_done = NULL;
        s_http_queue = NULL;
        vSemaphoreDelete(mutex);
        vSemaphoreDelete(done);
        vQueueDelete(queue);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t network_http_fetch(const char *url, const network_http_request_t *request,
                             const network_http_response_t **response)
{
    if (response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(&s_response, 0, sizeof(s_response));
    s_response.body = s_empty_body;
    *response = &s_response;
    if (url == NULL || request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *method_name = request->method != NULL ? request->method : "GET";
    if (!network_http_method_valid(method_name) || !network_http_timeout_valid(request->timeout_ms) ||
        !network_http_timeout_valid(request->total_timeout_ms) || request->total_timeout_ms < request->timeout_ms ||
        request->header_count > NETWORK_HTTP_HEADER_MAX || request->body_len > NETWORK_HTTP_BODY_MAX)
    {
        return request->body_len > NETWORK_HTTP_BODY_MAX ? ESP_ERR_INVALID_SIZE : ESP_ERR_INVALID_ARG;
    }
    if ((request->header_count > 0 && request->headers == NULL) || (request->body_len > 0 && request->body == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (request->body_len > 0 && !network_http_method_allows_body(method_name))
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t index = 0; index < request->header_count; ++index)
    {
        if (!network_http_header_valid(request->headers[index].name, request->headers[index].value))
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    const esp_err_t worker_err = ensure_http_worker();
    if (worker_err != ESP_OK)
    {
        return worker_err;
    }
    if (xSemaphoreTake(s_http_mutex, pdMS_TO_TICKS(request->total_timeout_ms)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_http_active || s_http_completed)
    {
        xSemaphoreGive(s_http_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_response, 0, sizeof(s_response));
    s_response.body = s_empty_body;
    s_response.url[0] = '\0';
    s_resp_body[0] = '\0';
    const esp_err_t compose_err = network_compose_url(url, s_req_url, sizeof(s_req_url));
    if (compose_err != ESP_OK || !wifi_is_connected())
    {
        xSemaphoreGive(s_http_mutex);
        return compose_err != ESP_OK ? compose_err : ESP_ERR_INVALID_STATE;
    }
    memcpy(s_response.url, s_req_url, strlen(s_req_url) + 1);

    if (request->body_len > 0)
    {
        memcpy(s_req_body, request->body, request->body_len);
    }
    s_req_body[request->body_len] = '\0';
    memset(s_req_headers, 0, sizeof(s_req_headers));
    for (size_t index = 0; index < request->header_count; ++index)
    {
        const size_t name_len = strlen(request->headers[index].name);
        const size_t value_len = strlen(request->headers[index].value);
        memcpy(s_req_headers[index].name, request->headers[index].name, name_len + 1);
        memcpy(s_req_headers[index].value, request->headers[index].value, value_len + 1);
    }

    while (xSemaphoreTake(s_http_done, 0) == pdTRUE)
    {
    }
    const int64_t deadline_us = esp_timer_get_time() + ((int64_t)request->total_timeout_ms * 1000);
    http_message_t message = {
        .request_id = ++s_next_request_id,
        .method = method_from_name(method_name),
        .timeout_ms = request->timeout_ms,
        .deadline_us = deadline_us,
        .body_len = request->body_len,
        .header_count = request->header_count,
    };
    s_http_active = true;
    s_http_abandoned = false;
    s_active_request_id = message.request_id;
    xSemaphoreGive(s_http_mutex);

    if (xQueueSend(s_http_queue, &message, 0) != pdTRUE)
    {
        xSemaphoreTake(s_http_mutex, portMAX_DELAY);
        s_http_active = false;
        xSemaphoreGive(s_http_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    const TickType_t remaining_ticks = time_until_deadline_ticks(deadline_us);
    if (remaining_ticks > 0 && xSemaphoreTake(s_http_done, remaining_ticks) == pdTRUE)
    {
        xSemaphoreTake(s_http_mutex, portMAX_DELAY);
        const esp_err_t err = s_http_completed && s_completed_request_id == message.request_id ? s_completed_error
                                                                                               : ESP_ERR_INVALID_STATE;
        if (s_http_completed && s_completed_request_id == message.request_id)
        {
            s_http_completed = false;
        }
        xSemaphoreGive(s_http_mutex);
        return err;
    }

    xSemaphoreTake(s_http_mutex, portMAX_DELAY);
    esp_err_t err = ESP_ERR_TIMEOUT;
    if (s_http_completed && s_completed_request_id == message.request_id)
    {
        err = s_completed_error;
        s_http_completed = false;
        (void)xSemaphoreTake(s_http_done, 0);
    }
    else if (s_http_active && s_active_request_id == message.request_id)
    {
        // R8/R2: the worker may still be inside esp_http_client_perform. Mark
        // abandoned so worker/event paths stop mutating the public response, and
        // expose a stable empty snapshot to the caller (body points to shared
        // static storage and must not be treated as live after timeout).
        s_http_abandoned = true;
        http_clear_public_response();
    }
    xSemaphoreGive(s_http_mutex);
    return err;
}
