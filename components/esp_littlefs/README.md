# esp_littlefs (vendored)

Vendored local copy of [joltwallet/esp_littlefs](https://github.com/joltwallet/esp_littlefs) **v1.23.1** (`4a4deda`) for COS.

## Scope
- Provides the LittleFS ESP-IDF VFS/partition integration and image-build hook used by `components/filesystem`.
- Upstream LittleFS core is vendored in `src/littlefs/` (BSD-3-Clause, see `src/littlefs/LICENSE.md`).
- ESP wrapper is MIT-licensed (see `LICENSE`).

## COS integration notes
- Resolved as a local component (`components/esp_littlefs`); not a managed dependency.
- `project_include.cmake` + `image-building-requirements.txt` (`littlefs-python==0.15.0`) are kept for `littlefs_create_partition_image(...)` (root `CMakeLists.txt`).
- `idf_component.yml` is kept as provenance/attribution metadata (origin, version, commit); it does not drive dependency resolution in this repo.
- Formatted and covered by `make format`/`format-check` and included in `make lint-db`; excluded from `make lint` (clang-tidy) due to upstream style vs project rules.
- Unity tests migrated to `integration_test/test_esp_littlefs/` (not run in every host check; requires ESP-IDF/QEMU/target as applicable).

## Maintenance
When updating, re-vendor from upstream tag/commit, preserve licenses and provenance, and revalidate filesystem mount/format and image generation behavior.
