#pragma once

#include "esp_err.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Maximum simultaneous WebSocket clients. */
#define WEBSOCKET_MAX_CLIENTS 4

    /**
     * @brief Initialize WebSocket: register /ws endpoint on the HTTP server
     *        and register the ws shell command.
     */
    esp_err_t websocket_init(void);

    /** Longest text frame accepted from a client, excluding the trailing newline. */
#define WEBSOCKET_MAX_TEXT 126

    /**
     * @brief Receive one client text frame after the authenticated handshake.
     *
     * `data` is not NUL-terminated. A missing trailing newline is added before
     * the handler runs, and the total length stays within one shell line.
     */
    typedef void (*websocket_text_handler_t)(const char *data, size_t len, void *context);

    /**
     * @brief Install or clear the incoming-text handler.
     * @param handler Callback, or NULL to ignore client text.
     * @param context Value passed through to the handler.
     */
    void websocket_set_text_handler(websocket_text_handler_t handler, void *context);

    /**
     * @brief Broadcast a text message to all connected WebSocket clients.
     *
     * Safe to call from any task. Failed sends remove the client.
     *
     * @param msg Message data.
     * @param len Message length.
     * @return ESP_OK on success.
     */
    esp_err_t websocket_broadcast(const char *msg, size_t len);

    /** @brief Number of currently connected WebSocket clients. */
    size_t websocket_client_count(void);

    /**
     * @brief Session close callback. Call from the HTTP server's close_fn.
     * @param sockfd The socket file descriptor being closed.
     */
    void websocket_on_session_close(int sockfd);

#ifdef __cplusplus
}
#endif
