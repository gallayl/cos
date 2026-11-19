# esp_littlefs (vendored)

Vendored local copy of [joltwallet/esp_littlefs](https://github.com/joltwallet/esp_littlefs) **v1.23.1** (`4a4deda`) for COS, with local stability/diagnostics hardening (diverges from upstream).

## Scope
- Provides the LittleFS ESP-IDF VFS/partition integration and image-build hook used by `components/filesystem`.
- Upstream LittleFS core is vendored in `src/littlefs/` (BSD-3-Clause, see `src/littlefs/LICENSE.md`).
- ESP wrapper is MIT-licensed (see `LICENSE`).

## COS integration notes
- Resolved as a local component (`components/esp_littlefs`); not a managed dependency.
- `project_include.cmake` + `image-building-requirements.txt` (`littlefs-python==0.15.0`) are kept for `littlefs_create_partition_image(...)` (root `CMakeLists.txt`).
- `idf_component.yml` is kept as provenance/attribution metadata (origin, version, commit); it does not drive dependency resolution in this repo.
- Formatted and covered by `make format`/`format-check` and included in `make lint-db`; excluded from `make lint` (clang-tidy) due to upstream style vs project rules.
- Unity tests migrated to `integration_test/test_esp_littlefs_core/` and `integration_test/test_esp_littlefs_extra/` (not run in every host check; requires ESP-IDF/QEMU/target as applicable).

## Behavior and limitations (COS hardening)
- **Logging:** hot-path per-operation start/done I/O traces (partition read/write/erase and VFS open/mkdir start/done) are downgraded to `ESP_LOGV`/removed to reduce noise and overhead; actionable `ESP_LOGE/W` diagnostics for hardware/error conditions remain.
- **Format safety:** `esp_littlefs_format*` / internal format paths reject unsafe format requests when the filesystem is mounted/active with `ESP_ERR_INVALID_STATE` and a log describing context. Callers must unmount/stop use before formatting; no in-place unmount/format/remount is performed while other operations may run. Formatting unmounted (including `dont_mount`) contexts remains supported.
- **Synchronization:** registry lookups/mutations are synchronized with the global EFS lock to reduce race/use-after-free risk in register/unregister/query/format paths.
- **Hang/deadlock limitations:** filesystem mutex waits still use `portMAX_DELAY` (no timeout semantic change). Unrecoverable hangs remain possible if a driver or lock holder stalls; bounded waits/timeout errors are deferred follow-up work if needed.
- **Diagnostics/error handling:** `F_GETPATH` reports truncation/failure as `-1` with `errno == ENAMETOOLONG`; mmap unmap is guarded to valid initialized mappings; backend operations include additional bounds/error checks and error context where practical; selected runtime-reachable asserts were converted to recoverable error handling.

## Maintenance
When updating, re-vendor from upstream tag/commit, preserve licenses/provenance and COS hardening intent, and revalidate filesystem mount/format and image generation behavior.
