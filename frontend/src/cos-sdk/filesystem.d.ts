/**
 * `fs` module. Listing is the only filesystem operation exposed to Berry.
 */

declare global {
  /**
   * Virtual filesystem listing.
   * Paths are absolute and must be `/`, `/flash`, `/sdcard`, or a descendant.
   * `..` segments are rejected. Paths are limited to 128 bytes.
   */
  namespace fs {
    /** One directory entry. Names are limited to 64 bytes. */
    interface DirectoryEntry {
      /** File or directory name, without its parent path. */
      name: string;
      /** Size in bytes. Directories are `0`. */
      size: number;
      /** `true` when the entry is a directory. */
      is_dir: boolean;
      /** Last modification time, in seconds since the Unix epoch. */
      mtime: number;
    }

    /** Successful listing. At most 64 entries are returned. */
    interface DirectoryListing {
      ok: true;
      entries: DirectoryEntry[];
      /** Number of entries in {@link entries}. */
      count: number;
    }

    /** Rejected path or a filesystem error. */
    interface DirectoryError {
      ok: false;
      /** ESP-IDF error name, such as `ESP_ERR_INVALID_ARG`. */
      error: string;
    }

    /**
     * List a virtual directory.
     *
     * @param path Absolute `/`, `/flash`, or `/sdcard` path.
     * @returns A listing when `ok` is true, otherwise an error name.
     */
    function list(path: string): DirectoryListing | DirectoryError;
  }
}

export {};
