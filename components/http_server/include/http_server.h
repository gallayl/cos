#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Initialize and start the HTTP server. Registers API endpoints and shell commands. */
    esp_err_t http_server_init(void);

    /** @brief Stop the HTTP server. */
    esp_err_t http_server_stop(void);

    /**
     * @brief Get the httpd handle for endpoint registration by other components.
     * @return The handle, or NULL if the server is not running.
     */
    httpd_handle_t http_server_get_handle(void);

    /* --- Pure utility functions (testable on host) --- */

    /**
     * @brief Sanitize an upload path.
     *
     * Normalizes slashes, rejects "..", and requires a /flash or /sdcard prefix.
     *
     * @param raw       Raw input path.
     * @param out_path  Output buffer for the sanitized path.
     * @param len       Size of the output buffer.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG if rejected.
     */
    esp_err_t http_sanitize_upload_path(const char *raw, char *out_path, size_t len);

    /**
     * @brief Get the MIME type for a file path based on its extension.
     * @param path  File path or name.
     * @return MIME type string (e.g. "text/html"). Never NULL.
     */
    const char *http_mime_type(const char *path);

    /* --- Static file serving --- */

    /** @brief Enable or disable static file serving. Persists to NVS. */
    esp_err_t http_static_set_enabled(bool enabled);

    /** @brief Check if static file serving is enabled. */
    bool http_static_is_enabled(void);

    /** @brief Set the virtual root directory for static files. Persists to NVS. */
    esp_err_t http_static_set_root(const char *virtual_path);

    /** @brief Get the current static root directory. */
    const char *http_static_get_root(void);

    /** @brief Enable or disable SPA fallback (serve index.html for unknown paths). Persists to NVS. */
    esp_err_t http_static_set_spa(bool enabled);

    /** @brief Check if SPA fallback is enabled. */
    bool http_static_is_spa(void);

    /* --- Authentication --- */

    /** @brief Set Basic auth credentials. Persists to NVS. */
    esp_err_t http_auth_set_credentials(const char *username, const char *password);

    /** @brief Get the current auth username. */
    const char *http_auth_get_username(void);

    /**
     * @brief Check Basic auth on a request.
     *
     * Returns ESP_OK if authorized. On failure, sends a 401 response with
     * WWW-Authenticate header and returns ESP_FAIL. Call this at the top
     * of every protected endpoint handler.
     */
    esp_err_t http_auth_check(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
