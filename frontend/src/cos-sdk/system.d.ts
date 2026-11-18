/**
 * `system` module.
 * Device system information and control.
 */

declare global {
  /** System and memory information. */
  namespace system {
    /** Heap/memory snapshot from the device (mirrors `mem`/`memory` command output). */
    interface MemInfoOk {
      ok: true;
      free_heap: number;
      min_free_heap: number;
      total_heap: number;
      max_alloc_block: number;
      internal_free_heap: number;
      internal_min_free_heap: number;
      internal_total_heap: number;
      internal_max_alloc_block: number;
    }

    interface MemInfoFailed {
      ok: false;
      /** ESP-IDF error name, e.g. `ESP_ERR_INVALID_ARG`. */
      error: string;
    }

    interface TimeInfoOk {
      ok: true;
      synced: boolean;
      epoch: number;
      /** ISO 8601 UTC time, e.g. `2026-02-18T14:30:00Z`. */
      utc: string;
      /** ISO 8601 local time with UTC offset, e.g. `2026-02-18T15:30:00+01:00`. */
      local: string;
    }

    interface TimeInfoFailed {
      ok: false;
      /** ESP-IDF/stable error name. */
      error: string;
    }

    /**
     * Read heap/memory statistics.
     * Use this from Berry scripts (e.g. command scripts) instead of parsing console output.
     */
    function meminfo(): MemInfoOk | MemInfoFailed;

    /**
     * Time since boot as a human-readable string (for example `45s`, `2m 30s`, `1h 23m 45s`).
     * Uses the same format as the `uptime` shell command.
     */
    function uptime(): string;

    /**
     * Read current time and sync status (read-only view; clock sync remains native).
     */
    function time_info(): TimeInfoOk | TimeInfoFailed;

    /**
     * Cooperatively sleep for `ms` milliseconds (R18 v1 async awaitable).
     * Returns a Promise that resolves with no value after the delay elapses under the runtime pump.
     * `ms <= 0` resolves on the next cooperative pump/drain opportunity. Invalid arguments fail
     * deterministically (no pending timer). Timer-cap overflow rejects with a deterministic error tag.
     */
    function sleep(ms: number): Promise<void>;
  }
}

export {};
