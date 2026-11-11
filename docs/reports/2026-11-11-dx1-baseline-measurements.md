# DX1 Baseline Measurements (developer tooling / scripts)

**Date:** 2026-11-11  
**Platform:** Windows (win32), workspace `E:\dev\cos`  
**Branch/status at capture:** `feat/r14-ts-diags...origin/feat/r14-ts-diags [ahead 1]` with local modifications in `.devcontainer/devcontainer.json`, `.vscode/settings.json` (pre-existing; not changed for this baseline)  
**Context:** Initial DX1 baseline pass per `docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md` (session task: start with baseline measurements, run scripts, measure execution time, track progress).

## Environment snapshot (available tools)

| Tool / check | Result |
|---|---|
| `make` | `c:\Program Files (x86)\GnuWin32\bin\make.exe` |
| `cmake` | Present (WinLibs) |
| `gcc` (host) | Present (WinLibs POSIX UCRT) |
| `python` | 3.14 (`C:\Users\gallayl\AppData\Local\Programs\Python\Python314\python.exe`) |
| `node` | `C:\Program Files\nodejs\node.exe` |
| `yarn` | `C:\Users\gallayl\AppData\Roaming\npm\yarn.ps1` (v1.22.22 observed in runs) |
| `clang-format` | Present (`...\Python314\Scripts\clang-format.exe`) |
| `clang-tidy` | `C:\Espressif\tools\esp-clang\esp-21.1.3_20260408\esp-clang\bin\clang-tidy.exe` |
| `idf.py` on PATH | Not found as direct command in this shell |
| `qemu-system-xtensa` on PATH | Not found |
| `berry` compiler executable | Not found (no `berry.exe`/`berry` discovered; only test binaries named `*berry*` under `test/build`) |
| ESP-IDF env vars (shell) | `IDF_PATH=C:/esp/v6.1/esp-idf/`, `IDF_TOOLS_PATH=C:/Espressif/tools`, `IDF_PYTHON_ENV_PATH=C:/Espressif/tools/python/v6.1/venv` observed; `idf.py` exists at `$IDF_PATH/tools/idf.py` |
| `frontend/node_modules` | Present |
| `build/` (firmware) | Present (existing artifacts) |
| `pytest` | 9.2.0 (`python -m pytest --version`) |

## Measured script/command baselines

Wall-clock times below are from `System.Diagnostics.Stopwatch` around the invoked command in this environment (single runs, warm/cold state as noted by context: existing `build/` and `frontend/node_modules` present; `make test` reported `ninja: no work to do`).

| ID | Command / script | Exit | Time (ms) | Result / notes |
|---:|---|---:|---:|---|
| B1 | `.\scripts\validate-berry-scripts.ps1 -SkipIfCompilerMissing` | 0 | 265 | **SKIP (warning):** Berry compiler not found. Script handled missing compiler path as expected with skip flag. Full validation (without skip) not run: no host Berry compiler located. |
| B2 | `make format-check` | 0 | 807 | Passed. Runs `clang-format --dry-run --Werror` over Makefile `SOURCES`. |
| B3 | `python scripts/integration_test.py build --help` | 0 | 206 | Passed (CLI/help path). Script entrypoint and argument parsing verified. |
| B4 | `make test` | 0 | 1908 | Passed. CMake configure/generate fast, Ninja `no work to do`, CTest **24/24 passed** (real ~0.25s) plus `python -m unittest discover -s scripts -p "test_*.py"` (**3 tests, 0.018s, OK**). |
| B5 | `frontend`: `yarn format:check` | 0 | 1534 | Passed (`prettier --check .`, "All matched files use Prettier code style!", ~1.29s reported by yarn). |
| B6 | `frontend`: `yarn lint` | 0 | 8245 | Passed (`eslint .`, ~8.00s reported by yarn). |
| B7 | `frontend`: `yarn test` | 0 | 2841 | Passed (`vitest run`: **19 files, 132 tests passed**, ~2.24s vitest / ~2.63s yarn). |
| B8 | `frontend`: `yarn build` | 0 | 5534 | Passed (`tsc && vite build`, built in ~2.60s / ~5.32s yarn). Vite warning emitted (large chunks >500 kB), not a failure. |

**Aggregate (measured commands B1–B8):** all executed commands exited 0 in this pass; total measured wall time ~21.34s (sum of individual stopwatch timings; sequential execution).

## Scripts/commands identified but not fully baseline-measured

| Script / command | Status | Reason / blocker | Suggested follow-up |
|---|---|---|---|
| `.\scripts\validate-berry-scripts.ps1` (without `-SkipIfCompilerMissing`) | Not measured | Berry host compiler not found in environment (`BERRY_EXE`/build locations/PATH). | Build pinned host Berry compiler from `build/berry-source` per `components/berry_lang/CMakeLists.txt`, set `BERRY_EXE` or pass `-BerryCompiler`, then time full `.be` validation (default `storage/flash_data`). |
| `python scripts/integration_test.py build [all\|<project>]` | Not measured | Full integration builds are ESP-IDF/QEMU-env dependent and potentially long/resource-heavy (6 projects); not executed in this baseline pass to avoid uncontrolled duration. ESP-IDF appears installed but full activated env/idf.py invocation not validated here. | Run in ESP-IDF v6.1 environment (e.g. devcontainer or activated shell). Capture clean+incremental timings, with `-j`/`INTEGRATION_TEST_JOBS` comparisons (DX4). |
| `python scripts/integration_test.py run [all\|<project>]` | Not measured | Requires built QEMU firmware images, `pytest-embedded` deps, and QEMU (`qemu-system-xtensa` not on PATH in this shell). | Measure per-project and full-suite after successful build; record env/cache state. |
| `.\scripts\r14-hardware-validation.ps1` | Not executed (by design) | Hardware/firmware validation helper; creates artifact directories/checklists and may run `idf.py` in `-FirmwareEval` mode. Not a generic software-only baseline script. | Use only during R14 hardware validation with target device/serial; time artifact-prep step separately if desired. |
| `make lint` | Not measured | Can be slow and requires prepared clang compile DB / ESP-IDF clang DB flow (`scripts/prepare_lint_db.py` -> `build/clang`); not run in this pass. | Measure after confirming `build/clang/compile_commands.json` freshness in ESP-IDF/clang-capable env; capture full vs scoped lint once DX3 adds modes. |
| `make build` / `make integration-test` | Not measured | Firmware build/integration orchestration depend on activated ESP-IDF toolchain and (for integration) QEMU; high cost. | Baseline in canonical env with clean/incremental runs. |
| Frontend `yarn install` / cold-cache scenarios | Not measured | `node_modules` already present. | Capture clean install (`--frozen-lockfile`) and cache-state variants separately. |

## Progress / tracking

- [x] Located DX task context: `docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md` (DX1 baseline/observability).
- [x] Inventoried environment and runnable scripts/tooling.
- [x] Ran and timed quick/safe script baselines (Berry validation skip, integration script help, format/test/lint/build commands available locally).
- [x] Ran and timed host tests (`make test`) and frontend CI-equivalent scripts (`yarn format:check`, `yarn lint`, `yarn test`, `yarn build`).
- [x] Recorded results in this report: `docs/reports/2026-11-11-dx1-baseline-measurements.md`.
- [ ] Full Berry compile validation baseline (needs host Berry compiler).
- [ ] Integration test build/run baselines (needs activated ESP-IDF v6.1 + QEMU + deps; measure `-j`/parallelism).
- [ ] `make lint` and firmware `make build` baselines (needs appropriate toolchain/compile DB).
- [ ] Cross-platform baselines (Linux/macOS/devcontainer) — not validated in this Windows-only pass.
- [ ] Peak RAM/disk/log-volume metrics and CI runner comparison (future DX1 refinement).

## Notes

- This pass did not modify firmware/frontend source logic; it executed existing checks/scripts to gather timing baselines.
- PowerShell/native-command stderr wrapping was observed in some captured outputs but did not affect exit codes for the passing commands above.
- Hardware-dependent validation was not performed.
