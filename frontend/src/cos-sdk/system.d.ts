/**
 * `system` module.
 * Device system information and control.
 */

declare global {
  /**
   * System information and lifecycle helpers for device scripts.
   */
  namespace system {
    /**
     * Time since boot as a human-readable string (for example `45s`, `2m 30s`, `1h 23m 45s`).
     * Uses the same format as the `uptime` shell command.
     */
    function uptime(): string;
  }
}

export {};
