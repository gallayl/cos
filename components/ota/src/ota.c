#include "ota.h"
#include "http_server.h"

#include "esp_console.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "ota";

static esp_ota_handle_t s_ota_handle = 0;
static const esp_partition_t *s_ota_partition = NULL;

static const char *OTA_HTML = "<html><head><title>COS OTA Update</title></head><body>"
                              "<h2>COS Firmware Update</h2>"
                              "<form method='POST' action='/update' enctype='multipart/form-data'>"
                              "<input type='file' name='firmware' accept='.bin'><br><br>"
                              "<input type='submit' value='Update'>"
                              "</form></body></html>";

static esp_err_t handler_ota_get(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, OTA_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t ota_chunk_cb(const uint8_t *data, size_t len, bool is_first, bool is_final, void *ctx)
{
    (void)ctx;

    if (is_first)
    {
        s_ota_partition = esp_ota_get_next_update_partition(NULL);
        if (!s_ota_partition)
        {
            ESP_LOGE(TAG, "No OTA partition found");
            return ESP_FAIL;
        }

        esp_err_t err = esp_ota_begin(s_ota_partition, OTA_WITH_SEQUENTIAL_WRITES, &s_ota_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "OTA update started (partition=%s)", s_ota_partition->label);
    }

    if (data && len > 0)
    {
        esp_err_t err = esp_ota_write(s_ota_handle, data, len);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            esp_ota_abort(s_ota_handle);
            return err;
        }
    }

    if (is_final)
    {
        esp_err_t err = esp_ota_end(s_ota_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
            return err;
        }

        err = esp_ota_set_boot_partition(s_ota_partition);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
            return err;
        }

        ESP_LOGI(TAG, "OTA update complete, restarting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    return ESP_OK;
}

static int cmd_ota(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running)
    {
        printf("Running:  %s (0x%08lx, %lu bytes)\n", running->label, (unsigned long)running->address,
               (unsigned long)running->size);
    }

    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (next)
    {
        printf("Next OTA: %s (0x%08lx, %lu bytes)\n", next->label, (unsigned long)next->address,
               (unsigned long)next->size);
    }
    else
    {
        printf("Next OTA: none\n");
    }

    return 0;
}

esp_err_t ota_init(void)
{
    httpd_handle_t server = http_server_get_handle();
    if (!server)
    {
        ESP_LOGE(TAG, "HTTP server not running");
        return ESP_ERR_INVALID_STATE;
    }

    const httpd_uri_t get_uri = {
        .uri = "/update",
        .method = HTTP_GET,
        .handler = handler_ota_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &get_uri);

    http_register_upload_handler("/update", ota_chunk_cb, NULL);

    const esp_console_cmd_t cmd = {
        .command = "ota",
        .help = "Show OTA partition info",
        .hint = NULL,
        .func = &cmd_ota,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    ESP_LOGI(TAG, "OTA endpoints registered");
    return ESP_OK;
}
