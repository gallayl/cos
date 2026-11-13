# Progress: Vendor esp_littlefs component (2026-11-13)

Plan: [docs/plan/2026-11-13-vendor-esp-littlefs-component.md](../plan/2026-11-13-vendor-esp-littlefs-component.md)

## Status
- [x] V1 Vendored `esp_littlefs` (+ upstream `src/littlefs/`) into `components/esp_littlefs/` from local `managed_components/joltwallet__littlefs` (v1.23.1). Removed nested git metadata and non-runtime dirs; kept core component sources, Kconfig, `project_include.cmake`, licenses.
- [x] V2 Wired `components/filesystem` to local component: added `esp_littlefs` to `REQUIRES`; removed managed dep from `components/filesystem/idf_component.yml`.
- [x] V3 Removed `joltwallet/littlefs` from root `dependencies.lock` and from integration test `dependencies.lock` files; removed stale `managed_components/joltwallet__littlefs` copies under `integration_test/`.
- [x] V4 Added `../../components/esp_littlefs` to `EXTRA_COMPONENT_DIRS` for integration tests using filesystem (`test_init_sequence`, `test_shell_fs`, `test_stability`, `test_system`, `test_vfs_ops`) so local component resolves alongside explicit component dirs. Root `littlefs_create_partition_image(...)` hook unchanged (provided by component `project_include.cmake`).
- [ ] V5 Build and hardware/QEMU-sensitive validation not fully run in this environment (ESP-IDF v6.1 required).

## Test migration (to project testing structure)
- [x] Migrated esp_littlefs Unity tests from vendored component (`test/`, `test_apps/`) into COS integration test app `integration_test/test_esp_littlefs/` (CMake, partitions, sdkconfig.defaults, `main/` with test sources + `test_main.c` running `unity_run_all_tests()`, and `test_esp_littlefs.py` pytest wrapper). Removed vendored component-local `test/` and `test_apps/` dirs.
- `shortcut: migrated Unity suite as an integration test app but not executed here (needs ESP-IDF build + QEMU/target), validate when environment is available`

## Notes / blockers
- Vendored source matches managed `joltwallet/littlefs` v1.23.1 present locally (IDF 6.1 target esp32, includes `esp_blockdev`/`littlefs_bdl.c` path). `make format` reformatted vendored C sources to satisfy repo clang-format checks.
- `make build` requires ESP-IDF v6.1 environment (`IDF_PATH`/tools sourced). Not run: ESP-IDF not confirmed/sourced in this session. Rerun: source IDF 6.1 env (e.g. `export.sh`/`idf_cmd_init`) and run `make build`.
- `integration_test/test_esp_littlefs` and existing filesystem integration tests require ESP-IDF v6.1, QEMU, and pinned deps in `integration_test/requirements.txt`. Not run here. Rerun with env ready: `make integration-test-build` and/or `pytest` per `integration_test/`.
- Hardware/QEMU-sensitive LittleFS mount-fail→format behavior not validated.

## Commands run
- `make format` — pass (reformatted vendored esp_littlefs sources to match repo style).
- `make format-check` — pass (exit 0).
- `make test` — pass (ctest: 24/24 passed; `python -m unittest discover -s scripts -p "test_*.py"`: 3 tests OK, exit 0).
