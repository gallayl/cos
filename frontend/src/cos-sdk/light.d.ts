/**
 * `light` module.
 * Ambient light sensor readings.
 */

declare global {
  /** Ambient light sensor access. */
  namespace light {
    /** Successful light sensor reading. */
    interface ReadOk {
      ok: true;
      /** Raw sensor value (0-4095). */
      value: number;
    }

    /** Failed or unavailable light sensor reading. */
    interface ReadFailed {
      ok: false;
      /** Error name, e.g. `ESP_FAIL`. */
      error: string;
    }

    /**
     * Read the current ambient light sensor value.
     * Use this from Berry/TS scripts instead of parsing console output.
     */
    function read(): ReadOk | ReadFailed;
  }
}

export {};
