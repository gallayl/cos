#pragma once

#include "esp_err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Maximum DNS hostname length in bytes. */
#define NETWORK_HOST_MAX 253
/** Minimum allowed ping deadline in milliseconds. */
#define NETWORK_PING_TIMEOUT_MIN_MS 100
/** Maximum allowed ping deadline in milliseconds. */
#define NETWORK_PING_TIMEOUT_MAX_MS 3000

    /** Result of one IPv4 ping attempt. */
    typedef struct
    {
        char address[16]; /**< Resolved IPv4 address, or empty before resolution. */
        uint32_t rtt_ms;  /**< Round-trip time in milliseconds. */
    } network_ping_result_t;

    /** Initialize the bounded network diagnostics service. */
    esp_err_t network_init(void);

    /**
     * Resolve an IPv4 host and send one ICMP echo request on the serialized
     * ping worker. The caller returns by timeout_ms, including DNS time.
     * A DNS lookup may continue on the worker after caller timeout, so a new
     * request is rejected until that lookup finishes.
     * @param host IPv4 address or hostname to resolve.
     * @param timeout_ms Overall caller deadline, including DNS resolution.
     * @param result Receives the resolved address and round-trip time.
     * @return ESP_OK on reply, or an ESP error on invalid input, timeout,
     *         unavailable Wi-Fi, or resolution failure.
     */
    esp_err_t network_ping(const char *host, uint32_t timeout_ms, network_ping_result_t *result);

#ifdef __cplusplus
}
#endif
