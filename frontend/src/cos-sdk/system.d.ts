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

    /**
     * Read heap/memory statistics.
     * Use this from Berry scripts (e.g. command scripts) instead of parsing console output.
     */
    function meminfo(): MemInfoOk | MemInfoFailed;
  }
}

export {};
