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

```bash
make integration-test-build INTEGRATION_TEST_BUILD_ARGS="test_shell_fs"
make integration-test-run INTEGRATION_TEST_RUN_ARGS="test_shell_fs"
```

Build/run all projects:

```bash
make integration-test-build
make integration-test-run
```

`scripts/integration_test.py` discovers `integration_test/test_*` projects. `pytest.ini` in this folder sets `--target esp32 --embedded-services idf,qemu`.

## Generated artifacts

Do not commit generated outputs. Local/regeneratable artifacts include `build/`, `managed_components/` (when fetched), `sdkconfig`, `sdkconfig.old`, `.pytest_cache/`, and `__pycache__/`. Clean or rebuild as needed; keep `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`, and test sources tracked.

## Isolation notes

Most tests assume a fresh QEMU boot/image for the test context (for example, `test_shell_fs` formats and seeds flash in `app_main`). When adding tests, prefer unique paths and explicit cleanup for mutating filesystem/shell state.
