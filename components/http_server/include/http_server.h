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

    /**
     * @brief Start the core HTTP server service (idempotent).
     *
     * Registers/starts httpd, endpoints, and the server command if needed.
     * Safe to call when already running.
     *
     * @return ESP_OK on success or if already running.
     */
    esp_err_t http_server_start(void);

    /** @brief Stop the HTTP server (best-effort). Safe when already stopped. */
    esp_err_t http_server_stop(void);

    /** @brief True if the HTTP server is running. */
    bool http_server_is_running(void);

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
     * @brief Decode a URL query value.
     *
     * Converts `%XX` escapes and `+` into spaces. The device filesystem API
     * receives paths through query strings, which `httpd_query_key_value()`
     * leaves encoded.
     *
     * @param src      Encoded query value.
     * @param dst      Output buffer for the decoded value.
     * @param dst_len  Size of the output buffer, including the terminator.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG for a bad escape, or
     *         ESP_ERR_INVALID_SIZE when the decoded value does not fit.
     */
    esp_err_t http_url_decode(const char *src, char *dst, size_t dst_len);

    /**
     * @brief Get the MIME type for a file path based on its extension.
     * @param path  File path or name.
     * @return MIME type string (e.g. "text/html"). Never NULL.
     */
    const char *http_mime_type(const char *path);

    /**
     * @brief Return whether a URI is reserved for API/WebSocket routes.
     *
     * Static file serving must not handle these routes. Returns true for paths
     * under `/api/` and for `/ws` (including `/ws/...` and legacy `/ws` prefix
     * matches) to preserve existing behavior.
     *
     * @param uri  Request URI (must be non-NULL for a meaningful result).
     * @return true if the URI is an API/WS route, false otherwise.
     */
    bool http_static_is_api_route(const char *uri);

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

    /** @brief Callback invoked when the HTTP server closes a client session socket. */
    typedef void (*http_session_close_cb_t)(int sockfd);

    /**
     * @brief Register a session-close callback.
     *
     * Used by components that maintain per-socket state (e.g. WebSocket).
     * Pass NULL to clear.
     */
    void http_server_set_session_close_cb(http_session_close_cb_t cb);

    /** @brief Clear the session-close callback only if it matches cb. */
    void http_server_clear_session_close_cb_if(http_session_close_cb_t cb);

    /** @brief Register a best-effort hook run before HTTP server teardown (e.g. stop dependent WebSocket). */
    void http_server_set_pre_stop_hook(void (*hook)(void));

#ifdef __cplusplus
}
#endif
