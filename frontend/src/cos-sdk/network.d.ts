/**
 * `net` module.
 * Wi-Fi must already have a station address before `ping` or `fetch`.
 * `wifi_connect` only starts the attempt; call `wifi` again after the address arrives.
 */

declare global {
  /**
   * Network access for device scripts.
   * DNS and ICMP share one worker. A lookup that outlives the deadline blocks
   * a later ping until the worker finishes. `fetch` uses its own worker.
   */
  namespace net {
    /** One ICMP echo completed. */
    interface PingOk {
      ok: true;
      /** Host name or address that was requested. */
      host: string;
      /** Resolved IPv4 address. */
      address: string;
      /** Round-trip time in milliseconds. */
      rtt_ms: number;
      /** Empty after a reply. */
      error: "";
    }

    /** The ping did not receive a reply. */
    interface PingFailed {
      ok: false;
      /** Host name or address that was requested. Empty when the argument was rejected. */
      host: string;
      /** Resolved IPv4 address, or empty when resolution did not finish. */
      address: string;
      /** Round-trip time in milliseconds, or `0` when there was no reply. */
      rtt_ms: number;
      /** ESP-IDF error name, such as `ESP_ERR_TIMEOUT` or `ESP_ERR_INVALID_ARG`. */
      error: string;
    }

    /**
     * Resolve an IPv4 host and send one ICMP echo request.
     * The call returns by the deadline, and that deadline includes DNS time.
     *
     * @param host Host name or IPv4 address.
     * @param timeout_ms Caller deadline in milliseconds, including DNS.
     * Defaults to `1000`. Values outside `100`–`3000` fail with
     * `ESP_ERR_INVALID_ARG`.
     */
    function ping(host: string, timeout_ms?: number): PingOk | PingFailed;

    /**
     * JSON value returned when `fetch` is asked for `response_type: "json"`.
     * Objects and arrays may nest up to the Berry parser limit of 25 levels.
     */
    type JsonValue = null | boolean | number | string | JsonValue[] | { [key: string]: JsonValue };

    /** Options for one HTTP request. Omitted fields use the defaults. */
    interface FetchRequest {
      /** Uppercase method. Defaults to `GET`. */
      method?: "GET" | "POST" | "PUT" | "PATCH" | "DELETE" | "HEAD";
      /**
       * Request headers. At most 8. Names are HTTP tokens of at most 40 bytes.
       * Values are printable ASCII of at most 256 bytes.
       */
      headers?: { [name: string]: string };
      /**
       * Request body, at most 4096 bytes.
       * `GET` and `HEAD` reject a body.
       */
      body?: string;
      /**
       * How to read a successful or error body.
       * `text` leaves `json` empty. `json` parses the body into `json`.
       * Defaults to `text`.
       */
      response_type?: "text" | "json";
      /**
       * Include the raw response body in the result. Defaults to `true`.
       * Set to `false` with `response_type: "json"` to skip creating the raw
       * `body` string when you only need the parsed `json` value.
       */
      include_body?: boolean;
      /**
       * Socket connect and read timeout in milliseconds.
       * Defaults to `10000`. Values outside `100`–`60000` are rejected.
       */
      timeout_ms?: number;
      /**
       * Wall-clock budget for DNS, connect, redirects, and the body.
       * Defaults to `timeout_ms` and must be at least that large.
       */
      total_timeout_ms?: number;
    }

    /** HTTP exchange that produced a 2xx status and, when requested, JSON. */
    interface FetchOk {
      ok: true;
      /** HTTP status code. */
      status: number;
      /** Absolute URL that was requested. */
      url: string;
      /** Response headers. Names are lowercase. At most 12 are kept. */
      headers: { [name: string]: string };
      /** Response body, at most 4096 bytes. Empty when `include_body` is `false`. */
      body: string;
      /** True when `json` holds the parsed document, including JSON `null`. */
      json_ok: boolean;
      /** Parsed document, or `null` when parsing was not requested or did not run. */
      json: JsonValue | null;
      /** Empty after success. */
      error: "";
    }

    /**
     * The request did not produce a usable 2xx result.
     * A transport failure has status `0`. An HTTP status outside 200–299 is
     * reported as `HTTP_<status>` and still carries the body. Invalid JSON
     * on a 2xx response is `ESP_ERR_INVALID_ARG`.
     */
    interface FetchFailed {
      ok: false;
      /** HTTP status, or `0` when no status line arrived. */
      status: number;
      /** Absolute URL, or the original argument when it could not be resolved. */
      url: string;
      /** Response headers. Empty when the connection failed. */
      headers: { [name: string]: string };
      /** Response body. Empty when the connection failed, the body exceeded 4096 bytes, or `include_body` is `false`. */
      body: string;
      /** True when a JSON body was parsed even though the status is not 2xx. */
      json_ok: boolean;
      /** Parsed error document, or `null` when there is no parsed JSON. */
      json: JsonValue | null;
      /**
       * `HTTP_<status>`, or an ESP-IDF name such as `ESP_ERR_TIMEOUT`,
       * `ESP_ERR_HTTP_CONNECT`, `ESP_ERR_INVALID_STATE`, or `ESP_ERR_INVALID_SIZE`.
       */
      error: string;
    }

    /**
     * Perform one HTTP request.
     * `http://` and `https://` URLs are absolute. A host containing a dot, such
     * as `example.com/path`, is requested as `http://example.com/path`.
     * Any other URL is resolved against the base set by {@link set_base_url}.
     * A path with a leading `/` replaces the base path. A relative path appends
     * to the base directory, so a base of `http://host/api/v1` plus `status`
     * requests `http://host/api/status`, while `http://host/api/v1/` plus
     * `status` requests `http://host/api/v1/status`.
     * HTTPS uses the common CA bundle. The response body is limited to 4096 bytes.
     *
     * @param url Absolute URL, host URL, or a URL relative to the base.
     * @param request Method, headers, body, response parsing, and timeouts.
     */
    function fetch(url: string, request?: FetchRequest): FetchOk | FetchFailed;

    /**
     * Remember the absolute URL used by a relative {@link fetch}.
     * Pass an empty string to clear it. The value is kept until reboot.
     * A query string is rejected.
     *
     * @param url `http` or `https` URL, or a host such as `example.com/api/`.
     * @returns `true`, or an ESP-IDF error name.
     */
    function set_base_url(url: string): true | string;

    /** Current base URL, or an empty string when none is set. */
    function base_url(): string;

    /** Station address, route, and DNS servers. */
    interface HostOk {
      ok: true;
      /** True when the station has an IPv4 address. */
      connected: boolean;
      /** Station address, or empty when it is 0.0.0.0. */
      ip: string;
      /** Subnet mask, or empty when it is 0.0.0.0. */
      netmask: string;
      /** Gateway, or empty when it is 0.0.0.0. */
      gateway: string;
      /** Up to two IPv4 DNS servers. */
      dns: string[];
      /** Station MAC address, `AA:BB:CC:DD:EE:FF`. */
      mac: string;
      /** Empty when the read succeeded. */
      error: "";
    }

    /** Wi-Fi is not running, or the call was rejected. */
    interface HostFailed {
      ok: false;
      /** ESP-IDF error name, such as `ESP_ERR_INVALID_STATE`. */
      error: string;
    }

    /**
     * Read the station address, netmask, gateway, DNS servers, and MAC.
     * Fields that are not available are empty. `connected` is false until
     * the station has an address.
     */
    function host(): HostOk | HostFailed;

    /** Current Wi-Fi association and the addresses tied to it. */
    interface WifiOk {
      ok: true;
      /** `NULL`, `STA`, `AP`, or `AP+STA`. */
      mode: string;
      /** True when the station has an IPv4 address. */
      connected: boolean;
      /** Associated access point, or the configured station SSID. */
      ssid: string;
      /** RSSI in dBm, or `0` when the station is not associated. */
      rssi: number;
      /** Signal label, or empty when the station is not associated. */
      signal: string;
      /** Authentication mode, or empty when the station is not associated. */
      auth: string;
      /** Station address, or empty when it is 0.0.0.0. */
      ip: string;
      /** Soft-AP address, or empty when it is 0.0.0.0. */
      ap_ip: string;
      /** Station MAC address. */
      mac: string;
      /** Empty when the read succeeded. */
      error: "";
    }

    /** Wi-Fi is not running, or the call was rejected. */
    interface WifiFailed {
      ok: false;
      /** ESP-IDF error name, such as `ESP_ERR_INVALID_STATE`. */
      error: string;
    }

    /** Read the Wi-Fi mode, association, signal, and addresses. */
    function wifi(): WifiOk | WifiFailed;

    /**
     * Start connecting to a network and store the credentials.
     * The call returns when the attempt starts. An address may arrive later;
     * read it with {@link wifi} or {@link host}.
     *
     * @param ssid Network name, 1–32 bytes.
     * @param password Network password, or omit it for an open network. At most 63 bytes.
     * @returns `true`, or an ESP-IDF error name.
     */
    function wifi_connect(ssid: string, password?: string): true | string;

    /**
     * Disconnect the station interface.
     * @returns `true`, or an ESP-IDF error name.
     */
    function wifi_disconnect(): true | string;
  }
}

export {};
