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
