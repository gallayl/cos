# Integration tests

ESP-IDF integration tests for COS, run on ESP32 QEMU with `pytest-embedded-idf` / `pytest-embedded-qemu`.

## Prerequisites

- ESP-IDF v6.1 environment sourced (`IDF_PATH`, tools on `PATH`)
- QEMU for Xtensa (`qemu-system-xtensa`) available to ESP-IDF tools / on `PATH`
- Python dependencies for the active ESP-IDF environment:

```bash
pip install -r integration_test/requirements.txt
```

## Build and run

From the repository root (preferred):

Build/run one suite (by short name or full project name):

```bash
make integration-test-build-shell_fs
make integration-test-run-shell_fs
make integration-test-shell_fs   # build + run
```

Available suites: `esp_littlefs`, `init_sequence`, `nvs_calibration`, `shell_fs`, `stability`, `system`, `vfs_ops`.

Build/run all projects:

```bash
make integration-test-build
make integration-test-run
```

You can still pass selectors/args via `INTEGRATION_TEST_BUILD_ARGS`, `INTEGRATION_TEST_RUN_ARGS`, and `INTEGRATION_TEST_JOBS`, or invoke `scripts/integration_test.py` directly. `scripts/integration_test.py` discovers `integration_test/test_*` projects. `pytest.ini` in this folder sets `--target esp32 --embedded-services idf,qemu`.

## Generated artifacts

Do not commit generated outputs. Local/regeneratable artifacts include `build/`, `managed_components/` (when fetched), `sdkconfig`, `sdkconfig.old`, `.pytest_cache/`, and `__pycache__/`. Clean or rebuild as needed; keep `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`, and test sources tracked.

## Isolation notes

Most tests assume a fresh QEMU boot/image for the test context (for example, `test_shell_fs` formats and seeds flash in `app_main`). When adding tests, prefer unique paths and explicit cleanup for mutating filesystem/shell state.
