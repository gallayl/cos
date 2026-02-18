#include "http_auth.h"

#include "esp_log.h"
#include "mbedtls/base64.h"
#include "nvs.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "http_auth";

#define NVS_NAMESPACE "http_auth"
#define NVS_KEY_USER "user"
#define NVS_KEY_PASS "pass"

#define DEFAULT_USER "cos"
#define DEFAULT_PASS "cos12345"

static char s_username[HTTP_AUTH_MAX_USER] = DEFAULT_USER;
static char s_password[HTTP_AUTH_MAX_PASS] = DEFAULT_PASS;

static esp_err_t load_credentials(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    size_t len = sizeof(s_username);
    if (nvs_get_blob(handle, NVS_KEY_USER, s_username, &len) != ESP_OK)
    {
        strncpy(s_username, DEFAULT_USER, sizeof(s_username) - 1);
    }
    s_username[sizeof(s_username) - 1] = '\0';

    len = sizeof(s_password);
    if (nvs_get_blob(handle, NVS_KEY_PASS, s_password, &len) != ESP_OK)
    {
        strncpy(s_password, DEFAULT_PASS, sizeof(s_password) - 1);
    }
    s_password[sizeof(s_password) - 1] = '\0';

    nvs_close(handle);
    return ESP_OK;
}

esp_err_t http_auth_init(void)
{
    load_credentials();
    ESP_LOGI(TAG, "Auth initialized (user=%s)", s_username);
    return ESP_OK;
}

esp_err_t http_auth_set_credentials(const char *username, const char *password)
{
    if (username == NULL || password == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(username) == 0 || strlen(username) >= HTTP_AUTH_MAX_USER)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(password) == 0 || strlen(password) >= HTTP_AUTH_MAX_PASS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(s_username, username, sizeof(s_username) - 1);
    s_username[sizeof(s_username) - 1] = '\0';
    strncpy(s_password, password, sizeof(s_password) - 1);
    s_password[sizeof(s_password) - 1] = '\0';

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }
    nvs_set_blob(handle, NVS_KEY_USER, s_username, strlen(s_username) + 1);
    nvs_set_blob(handle, NVS_KEY_PASS, s_password, strlen(s_password) + 1);
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Credentials updated (user=%s)", s_username);
    return ESP_OK;
}

const char *http_auth_get_username(void)
{
    return s_username;
}

esp_err_t http_auth_check(httpd_req_t *req)
{
    char auth_hdr[256] = {0};
    esp_err_t err = httpd_req_get_hdr_value_str(req, "Authorization", auth_hdr, sizeof(auth_hdr));
    if (err != ESP_OK)
    {
        goto unauthorized;
    }

    if (strncmp(auth_hdr, "Basic ", 6) != 0)
    {
        goto unauthorized;
    }

    const char *b64 = auth_hdr + 6;
    uint8_t decoded[128];
    size_t decoded_len = 0;
    int ret = mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &decoded_len, (const uint8_t *)b64, strlen(b64));
    if (ret != 0)
    {
        goto unauthorized;
    }
    decoded[decoded_len] = '\0';

    char *colon = strchr((char *)decoded, ':');
    if (colon == NULL)
    {
        goto unauthorized;
    }
    *colon = '\0';

    const char *req_user = (const char *)decoded;
    const char *req_pass = colon + 1;

    if (strcmp(req_user, s_username) != 0 || strcmp(req_pass, s_password) != 0)
    {
        goto unauthorized;
    }

    return ESP_OK;

unauthorized:
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"COS\"");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
}
