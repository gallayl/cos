/**
 * `fs` module for the virtual filesystem.
 *
 * A path is absolute when it starts with `/`, and relative to the running
 * script's directory when it starts with `./`. `./../` climbs to the parent
 * directory. Any other form raises `value_error: path must start with '/' or './'`.
 * A relative path raises `value_error: relative paths need a script file` when
 * the code was not started from a script, such as `berry eval`.
 * `..` that climbs above `/` raises `value_error: path escapes the filesystem root`.
 * A path longer than 127 bytes raises `value_error: path is too long`.
 * These failures stop the script. Scripts cannot catch them.
 *
 * `/`, `/flash`, `/sdcard`, and their descendants are the only accepted
 * locations. Another absolute path fails the call with `ESP_ERR_INVALID_ARG`.
 */

declare global {
  /**
   * Virtual filesystem used by device scripts.
   * Text and JSON documents are limited to 4096 bytes.
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
      /** Creation or status-change time, in seconds since the Unix epoch. */
      ctime: number;
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

    /** Text read from a file. */
    interface TextFile {
      ok: true;
      /** File contents. Embedded NUL bytes are rejected. */
      text: string;
    }

    /**
     * JSON value stored by {@link write_json}.
     * Objects and arrays may nest up to the Berry parser limit of 25 levels.
     */
    type JsonValue = null | boolean | number | string | JsonValue[] | { [key: string]: JsonValue };

    /** JSON value read from a file. */
    interface JsonFile {
      ok: true;
      /** Parsed document. JSON `null` is `null`. */
      value: JsonValue;
    }

    /** Failed text, JSON, or size call. */
    interface FileError {
      ok: false;
      /** ESP-IDF error name, such as `ESP_ERR_NOT_FOUND` or `ESP_ERR_INVALID_SIZE`. */
      error: string;
    }

    /** Size of a regular file. */
    interface FileSize {
      ok: true;
      /** Size in bytes. */
      size: number;
    }

    /**
     * List a virtual directory.
     *
     * @param path Absolute `/`, `/flash`, or `/sdcard` path, or a `./` path
     * relative to the script.
     * @returns A listing when `ok` is true, otherwise an error name.
     */
    function list(path: string): DirectoryListing | DirectoryError;

    /**
     * Read a text file of at most 4096 bytes.
     *
     * @param path File path. Directories are rejected.
     * @returns The text when `ok` is true. `ESP_ERR_INVALID_SIZE` means the
     * file is larger than 4096 bytes. `ESP_ERR_INVALID_ARG` means the file
     * contains a NUL byte or is a directory.
     */
    function read_text(path: string): TextFile | FileError;

    /**
     * Create or replace a text file. The parent directory must already exist.
     *
     * @param path File path.
     * @param text Contents, at most 4096 bytes.
     * @returns `true` when the file is written. Otherwise an ESP-IDF error name.
     */
    function write_text(path: string, text: string): true | string;

    /**
     * Read a JSON file of at most 4096 bytes.
     * Leading and trailing spaces, tabs, and newlines are ignored.
     *
     * @param path File path.
     * @returns The value when `ok` is true. `ESP_ERR_INVALID_ARG` means the
     * file is not JSON. `ESP_ERR_INVALID_SIZE` means it is larger than 4096 bytes.
     */
    function read_json(path: string): JsonFile | FileError;

    /**
     * Write a value as one JSON document, replacing an existing file.
     * The parent directory must already exist. Objects, arrays, strings,
     * numbers, booleans, and `null` may be nested.
     *
     * @param path File path.
     * @param value Value to encode. The encoded document must be at most 4096 bytes.
     * @returns `true` when the file is written. Otherwise an ESP-IDF error name.
     */
    function write_json(path: string, value: JsonValue): true | string;

    /**
     * Report whether a path exists.
     * A missing file and a path outside `/flash` and `/sdcard` are both `false`.
     * A path that is not a string raises `value_error: path must be a string`.
     */
    function exists(path: string): boolean;

    /**
     * Report whether a path is a directory.
     * A missing path or a file is `false`.
     */
    function is_dir(path: string): boolean;

    /**
     * Report whether a path is a regular file.
     * A missing path or a directory is `false`.
     */
    function is_file(path: string): boolean;

    /**
     * Read the size of a regular file.
     *
     * @param path File path.
     * @returns The size when `ok` is true. A directory or missing path fails.
     */
    function size(path: string): FileSize | FileError;
  }
}

export {};
