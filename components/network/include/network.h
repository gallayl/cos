#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
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

    /**
     * Resolve an IPv4 host on the same serialized worker used by network_ping.
     * The caller returns by timeout_ms, including DNS time. A lookup may
     * continue on the worker after caller timeout, so a new request is
     * rejected until that lookup finishes.
     */
    esp_err_t network_resolve(const char *host, uint32_t timeout_ms, network_ping_result_t *result);

/** Maximum absolute URL length in bytes, excluding the terminating NUL. */
#define NETWORK_URL_MAX 512
/** Maximum HTTP request or response body size in bytes. */
#define NETWORK_HTTP_BODY_MAX 4096
/** Maximum request headers accepted by one fetch. */
#define NETWORK_HTTP_HEADER_MAX 8
/** Maximum response headers retained from one fetch. */
#define NETWORK_HTTP_RESPONSE_HEADER_MAX 12
/** Maximum HTTP header name length in bytes. */
#define NETWORK_HTTP_HEADER_NAME_MAX 40
/** Maximum HTTP header value length in bytes. */
#define NETWORK_HTTP_HEADER_VALUE_MAX 256
/** Shortest accepted fetch timeout in milliseconds. */
#define NETWORK_HTTP_TIMEOUT_MIN_MS 100
/** Longest accepted fetch timeout in milliseconds. */
#define NETWORK_HTTP_TIMEOUT_MAX_MS 60000
/** Fetch timeout used when a script omits both timeout fields. */
#define NETWORK_HTTP_TIMEOUT_DEFAULT_MS 10000

    /** One HTTP header name and value. Both are NUL-terminated. */
    typedef struct
    {
        char name[NETWORK_HTTP_HEADER_NAME_MAX + 1];   /**< Header name. */
        char value[NETWORK_HTTP_HEADER_VALUE_MAX + 1]; /**< Header value. */
    } network_http_header_t;

    /** One HTTP header name and value. Both strings are owned by the caller. */
    typedef struct
    {
        const char *name;
        const char *value;
    } network_http_request_header_t;

    /** Inputs for one HTTP fetch. Pointers are only read during the call. */
    typedef struct
    {
        const char *method; /**< `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, or `HEAD`. NULL means GET. */
        const char *body;   /**< Request body, or NULL when body_len is 0. */
        size_t body_len;    /**< Body length in bytes. */
        const network_http_request_header_t *headers; /**< Request headers, or NULL when header_count is 0. */
        size_t header_count;                          /**< Number of request headers. */
        uint32_t timeout_ms;                          /**< Socket connect and read timeout. */
        uint32_t total_timeout_ms;                    /**< Wall-clock budget for the whole call, including DNS. */
    } network_http_request_t;

    /**
     * Completed HTTP exchange.
     * `body` points at storage owned by the network component and stays valid
     * until the next network_http_fetch() call.
     */
    typedef struct
    {
        int status;                    /**< HTTP status, or 0 when no status arrived. */
        char url[NETWORK_URL_MAX + 1]; /**< Absolute URL that was requested. */
        network_http_header_t headers[NETWORK_HTTP_RESPONSE_HEADER_MAX]; /**< Response headers. Names are lowercase. */
        size_t header_count;                                             /**< Number of stored response headers. */
        const char *body; /**< Response body. Empty when the call failed in transport. */
        size_t body_len;  /**< Body length in bytes. */
    } network_http_response_t;

    /** STA address, route, and DNS servers currently visible to the device. */
    typedef struct
    {
        bool connected;    /**< True when the STA has an IP address. */
        char ip[16];       /**< STA IPv4 address, or empty when it is 0.0.0.0. */
        char netmask[16];  /**< STA subnet mask, or empty when it is 0.0.0.0. */
        char gateway[16];  /**< STA gateway, or empty when it is 0.0.0.0. */
        char dns[2][16];   /**< Configured IPv4 DNS servers. */
        uint8_t dns_count; /**< Number of entries in dns[]. */
        char mac[18];      /**< STA MAC address, `AA:BB:CC:DD:EE:FF`. */
    } network_host_info_t;

    /** Wi-Fi mode, association, and the addresses tied to it. */
    typedef struct
    {
        char mode[8];    /**< `NULL`, `STA`, `AP`, or `AP+STA`. */
        bool connected;  /**< True when the STA has an IP address. */
        char ssid[33];   /**< Associated or configured STA SSID. */
        int8_t rssi;     /**< Associated AP RSSI in dBm, or 0 when not associated. */
        char signal[16]; /**< Signal label, or empty when not associated. */
        char auth[20];   /**< Associated AP authentication mode, or empty. */
        char ip[16];     /**< STA IPv4 address, or empty when it is 0.0.0.0. */
        char ap_ip[16];  /**< Soft-AP IPv4 address, or empty when it is 0.0.0.0. */
        char mac[18];    /**< STA MAC address. */
    } network_wifi_info_t;

    /**
     * Remember the absolute URL used to resolve a relative fetch URL.
     * An empty string clears it. The value is kept in RAM until reboot.
     * A query string is rejected. A path without a trailing slash replaces
     * its last segment when a relative URL is joined.
     * @param url Absolute `http` or `https` URL, a host such as `example.com/api`, or empty.
     * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_SIZE.
     */
    esp_err_t network_set_base_url(const char *url);

    /**
     * Copy the current base URL.
     * @param out Receives the base URL, or an empty string when none is set.
     * @param out_size Size of out, including space for the NUL.
     * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_SIZE.
     */
    esp_err_t network_get_base_url(char *out, size_t out_size);

    /**
     * Resolve a script URL to an absolute `http` or `https` URL.
     * `http://` and `https://` are absolute. A host that contains a dot, such
     * as `example.com/path`, is treated as `http://example.com/path`. Any other
     * form is resolved against the base URL.
     * @param url Absolute URL or a reference relative to the base URL.
     * @param out Receives the absolute URL.
     * @param out_size Size of out, including space for the NUL.
     * @return ESP_OK on success. ESP_ERR_INVALID_STATE when a relative URL has
     *         no base. ESP_ERR_INVALID_ARG or ESP_ERR_INVALID_SIZE otherwise.
     */
    esp_err_t network_compose_url(const char *url, char *out, size_t out_size);

    /**
     * Report whether a method name is one fetch accepts.
     * @param method Uppercase method name. NULL is rejected.
     */
    bool network_http_method_valid(const char *method);

    /**
     * Report whether a method may carry a request body.
     * GET and HEAD cannot. NULL is rejected.
     */
    bool network_http_method_allows_body(const char *method);

    /**
     * Report whether a header name and value fit the fetch limits.
     * Names are HTTP tokens. Values are printable ASCII without CR or LF.
     */
    bool network_http_header_valid(const char *name, const char *value);

    /** Report whether a timeout is inside the accepted inclusive range. */
    bool network_http_timeout_valid(uint32_t timeout_ms);

    /**
     * Perform one HTTP request on the network worker.
     * Wi-Fi must already have a STA address. The call returns when the status
     * line arrives, the transport fails, or total_timeout_ms elapses.
     * ESP_OK means an HTTP status was received, including statuses outside
     * 200–299. The caller decides how to treat that status.
     * A timed-out call may leave the worker busy; the next call then returns
     * ESP_ERR_INVALID_STATE until the worker finishes.
     * @param url Absolute or base-relative URL.
     * @param request Method, headers, body, and timeouts.
     * @param response Receives the exchange. Never NULL on return when this
     *        pointer was not NULL, including on failure.
     * @return ESP_OK when a status line was received, otherwise an ESP error.
     */
    esp_err_t network_http_fetch(const char *url, const network_http_request_t *request,
                                 const network_http_response_t **response);

    /**
     * Read the STA address, gateway, DNS servers, and MAC.
     * @param info Receives the fields that are currently available.
     * @return ESP_OK, or ESP_ERR_INVALID_STATE when Wi-Fi is not running.
     */
    esp_err_t network_get_host_info(network_host_info_t *info);

    /**
     * Read the current Wi-Fi mode, association, and addresses.
     * @param info Receives the available Wi-Fi fields.
     * @return ESP_OK, or ESP_ERR_INVALID_STATE when Wi-Fi is not running.
     */
    esp_err_t network_get_wifi_info(network_wifi_info_t *info);

    /**
     * Start a STA connection and store the credentials.
     * Returns when association is requested, not when an address arrives.
     * @param ssid Station SSID, 1–32 bytes.
     * @param password Station password, or NULL for an open network. At most 63 bytes.
     * @return ESP_OK when the connection attempt starts, otherwise an ESP error.
     */
    esp_err_t network_wifi_connect(const char *ssid, const char *password);

    /**
     * Disconnect the STA interface.
     * @return ESP_OK, or the Wi-Fi error when the interface is not running.
     */
    esp_err_t network_wifi_disconnect(void);

#ifdef __cplusplus
}
#endif
