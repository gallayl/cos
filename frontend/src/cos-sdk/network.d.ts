/**
 * `net` module. Wi-Fi must already be connected.
 */

declare global {
  /**
   * Network diagnostics.
   * DNS and ICMP share one worker. A lookup that outlives the deadline blocks
   * a later ping until the worker finishes.
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
  }
}

export {};
