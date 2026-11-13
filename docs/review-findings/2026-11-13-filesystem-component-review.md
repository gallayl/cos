# Filesystem component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/filesystem/` (`include/filesystem.h`, `src/flash.c`, `src/flash.h`, `src/sdcard.c`, `src/sdcard.h`, `src/vfs.c`, `src/vfs_path.c`, `src/cyd_sdcard_pins.h`, `CMakeLists.txt`, `Kconfig`, `idf_component.yml`)  
**Reviewer focus:** correctness, safety, resource handling, path resolution, data writes, and maintainability of the filesystem/VFS module as-is.

## What this repo does
COS is ESP32 (CYD/ESP32-2432S028R) firmware with a display/touch UI and an interactive console. The `filesystem` component mounts LittleFS (`/flash` virtual root backed by `/littlefs`) and optionally an SD card over SPI (`/sdcard`), and exposes virtual-path helpers plus file operations (`list/read/stream/write/copy/rename/mkdir/remove/stat`) used by shell commands, HTTP server file serving/upload, Berry scripts/modules, and other components.

**Load assumed:** single-user embedded device; filesystem calls come mainly from the console/shell and occasional background/runtime users (for example HTTP server and Berry) one operation at a time, on small files and directories. Not designed as a high-concurrency multi-client storage service.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| F1 | Must fix | `vfs_resolve_path` does not normalize `.`/`..`, allowing virtual-path escape | Open |  | Prefix-mapped real path can traverse outside mount root if caller passes unnormalized input |
| F2 | Should fix | `vfs_read_file`, `vfs_write_file`, and `vfs_read_file_stream` use text mode, which can corrupt binary data | Fixed |  | 2026-11-13: `components/filesystem/src/vfs.c` now uses binary modes (`"rb"` for reads/stream, `"wb"` for writes); copy path reads via stream and writes `"wb"`. |
| F3 | Should fix | VFS/file operation logic has no host tests for `vfs.c` behavior (only `vfs_path.c` covered) | Open |  | `test/test_vfs_path.c` exercises path helpers; no host tests for read/write/copy/rename/remove/list/stat/root semantics |
| F4 | Nice to have | Small cleanup opportunities in init/deinit wrappers | Open |  | `flash_deinit` redundant unregister branch; `sdcard_init` only forwards to `sdcard_mount` |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### Must fix

#### 1. **`vfs_resolve_path` does not normalize `.`/`..`, allowing virtual-path escape** (`components/filesystem/src/vfs_path.c:158-196`, `components/filesystem/src/vfs.c:80`, `127`, `152`, `202`, `279`, `315`, `320`, `362`, `395`, `444`, `479`, `521`)
- **What this is:** `vfs_resolve_path()` maps `/flash/...` and `/sdcard/...` virtual paths to real mount paths (`FLASH_MOUNT_POINT`/`SDCARD_MOUNT_POINT` plus the suffix) and is the common entry point for all VFS file operations. `vfs_compose_script_path()` separately normalizes script-relative/absolute input and rejects `..` above `/`.
- **Problem:** `vfs_resolve_path()` copies the suffix after `/flash` or `/sdcard` verbatim and never normalizes `.`/`..` or rejects `..` segments. A virtual path such as `/flash/../sdcard/secret` or `/sdcard/../../...` passes `vfs_is_virtual_path()` (prefix match) and resolves to a real path like `/littlefs/../sdcard/secret`, which can traverse outside the intended mount root when processed by the underlying filesystem. Callers that accept user-controlled virtual paths and do not pre-normalize (for example shell, HTTP, Berry paths passed directly to VFS APIs) can therefore read, write, rename, or delete outside the selected virtual area if the platform follows those segments.
- **Fix:** Normalize the virtual path before mapping it (reuse the same `.`/`..` rules as `vfs_compose_script_path`/`normalize_absolute`, or share a helper), reject any path that climbs above the virtual root, and keep mount selection based on the normalized `/flash` or `/sdcard` prefix. Add tests for traversal inputs like `/flash/../x`, `/flash/a/../../x`, and `/sdcard/../flash/x`.
- **If we skip it:** A crafted or mistaken path containing `..` can escape the intended `/flash` or `/sdcard` sandbox and cause reads/writes/deletes on the wrong location, which is a data-loss and integrity risk.

### Should fix

#### 2. **`vfs_read_file`, `vfs_write_file`, and `vfs_read_file_stream` use text mode, which can corrupt binary data** (`components/filesystem/src/vfs.c:133-142`, `158-192`, `208-225`; compare `vfs_copy_file` at `285-295`)
- **What this is:** The VFS helpers open files for whole-file reads (`vfs_read_file`), chunked streaming reads (`vfs_read_file_stream`), full writes (`vfs_write_file`), and bounded copies (`vfs_copy_file`).
- **Problem:** `vfs_read_file` and `vfs_read_file_stream` open with `"r"` and `vfs_write_file` opens with `"w"`, while `vfs_copy_file` writes the destination with `"wb"` and streams from the source through the `"r"` read path. On targets/libc configurations where text mode translates line endings or treats control characters specially, binary content can be changed on read or write, and copy behavior can differ from read+write helpers. The API (`char *buf`, raw `data/len`) is otherwise byte-oriented.
- **Fix:** Use binary modes consistently for byte-oriented operations (`"rb"` for reads/stream source, `"wb"` for writes/copy destination) unless a text-mode API is explicitly intended. Keep behavior the same for plain text files.
- **If we skip it:** Binary files (images, compiled assets, arbitrary uploads/downloads handled through these helpers) may be corrupted or read back with altered bytes on affected environments.

#### 3. **VFS/file operation logic has no host tests for `vfs.c` behavior (only `vfs_path.c` covered)** (`components/filesystem/src/vfs.c`, `test/test_vfs_path.c`, `test/CMakeLists.txt`)
- **What this is:** `vfs.c` implements virtual-root listing, read/stream/write, copy with cleanup, same-mount rename with overwrite refusal, mkdir/remove semantics, and existence/stat helpers. `test/test_vfs_path.c` (built via `libvfs_path.a` from `src/vfs_path.c`) covers `vfs_resolve_path`, `vfs_compose_script_path`, and `vfs_is_virtual_path`.
- **Problem:** There are no host unit tests for the operational behavior in `vfs.c` (for example `/` and `/flash`/`/sdcard` special cases, `vfs_copy_file` refusal/cleanup on failure, `vfs_rename_file` same-mount and no-overwrite rules, `vfs_mkdir` existing-directory handling, `vfs_remove` directory vs file behavior, and stream callback/error propagation). These paths include data-write and error-handling logic that is easy to regress.
- **Fix:** Add host-based tests for the pure/risky `vfs.c` behaviors using the existing mock filesystem/host test patterns (or a small temp-dir-backed harness where practical), starting with copy/rename/remove/mkdir/stat/list edge cases and error paths.
- **If we skip it:** Future changes can silently break file operations or safety checks (overwrite refusal, partial-copy cleanup, mount rules) without CI catching it.

### Nice to have

#### 4. **Small cleanup opportunities in init/deinit wrappers** (`components/filesystem/src/flash.c:68-76`, `84-87`, `components/filesystem/src/sdcard.c:55-58`)
- **What this is:** Flash mount/unmount/format helpers manage LittleFS registration state; `sdcard_init()` is the public SD init wrapper.
- **Problem:** `flash_deinit()` unregisters and then unconditionally unregisters again on the not-mounted path (double call with ignored error), and `flash_format()` also performs extra unregister/deinit steps around formatting. `sdcard_init()` only forwards to `sdcard_mount()` with no additional behavior. These do not show a concrete runtime bug, but they add small redundant branches/wrappers.
- **Fix:** Collapse the redundant unregister paths in `flash_deinit()`/`flash_format()` into one clear cleanup sequence and consider whether the `sdcard_init()` alias is still needed by callers (or keep it as a thin documented alias if required by the API).
- **If we skip it:** No runtime impact expected; code is just slightly harder to read and maintain.

## Validation / not checked

- 2026-11-13: Ran `make format-check` and `make test` after F2 fix — **pass** (24 host ctest + 3 script tests). Did not run ESP-IDF firmware build/lint or on-device validation for this change.
- ESP-IDF firmware build/lint and on-device behavior (ESP-IDF toolchain, LittleFS partition, SD card/SPI hardware not exercised in the original review).
- `make format-check` and `make test` were not run as part of the original read-only audit.
- Runtime behavior of underlying dependencies/APIs (`esp_littlefs`, FAT/SDMMC/SPI drivers, POSIX VFS semantics, storage layout) beyond source inspection.
- Concurrency/thread-safety: filesystem APIs and SD mount state (`s_mounted`, `s_card`, `s_spi_initialized`) were not audited for simultaneous access from multiple tasks; the module is assumed to be used in a low-concurrency embedded context.
- Actual exposure of traversal/untrusted paths through every caller (shell, HTTP server, Berry, UI) beyond the generic API risk described above.

## Related code/tests

- Implementation: `components/filesystem/src/*`, `components/filesystem/include/filesystem.h`
- Build/config: `components/filesystem/CMakeLists.txt`, `components/filesystem/Kconfig`, `components/filesystem/idf_component.yml`
- Existing host tests: `test/test_vfs_path.c` (covers `vfs_path.c` helpers), filesystem mocks in `test/mocks/mock_filesystem.{c,h}`
- Notable callers (from repo search): `main/main.c` (`filesystem_init`), `components/shell/src/shell_fs_cmds.c`, `components/shell/src/shell_sd_cmds.c`, `components/shell/src/shell_redirection.c`, `components/berry_lang/src/berry_runtime.cpp`, `components/http_server/src/http_static.c`, `components/http_server/src/http_upload.c`

## Verdict
The filesystem module is small and clear, but path resolution currently trusts unnormalized virtual paths, so `..` segments can escape the intended `/flash` or `/sdcard` area. File I/O helpers also use text mode for byte-oriented reads/writes, and the operational VFS logic in `vfs.c` lacks host-test coverage.

**Lean: -6 lines, -0 dependencies possible.** (Approximate: collapse redundant unregister/forwarding branches in `flash.c`/`sdcard.c`; no safe dependency or public-API removal confirmed without broader caller checks.)

## Not checked
ESP-IDF/device validation, host unit/integration test runs, runtime behavior of LittleFS/FAT/SPI/storage dependencies, and multi-task/concurrent access patterns beyond source inspection.
