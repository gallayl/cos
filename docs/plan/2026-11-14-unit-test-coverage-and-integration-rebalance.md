# Unit test coverage gaps and integration-to-unit rebalancing

**Created:** 2026-11-14  
**Status:** Proposed / analysis (not started)  
**Scope:** Host unit tests (`test/`), integration tests (`integration_test/`), and component testability seams  
**Related:** [`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md) (R25, esp. #3, #4, #6, #11), `docs/plan/consolidated-improvement-plan.md`, `test/CMakeLists.txt`, `integration_test/README.md`, `AGENTS.md`

## Problem

The project has meaningful host unit coverage for many isolated modules (mocks + Unity targets in `test/CMakeLists.txt`) and a growing ESP32/QEMU integration suite (`integration_test/test_*`), but coverage is uneven:

- Several components with non-trivial logic have little or no host-level tests, so regressions surface late in QEMU/device runs.
- Some integration scenarios primarily exercise pure logic, contract/argument handling, or shell/Berry command behavior that can be validated faster and more deterministically with host tests and mocks.
- Keeping the right boundary (unit for logic/edges, integration for real FS/NVS/boot/runtime wiring) is important on constrained targets and for CI turnaround.

This document is the detailed coverage inventory for **R25 #11 (coverage enablement)** and complements R25 refactor/seam items (#3 HTTP helper extraction, #4 testability seams/reset hooks, #6 `network_http` decomposition) by identifying where those enablers unlock earlier-failing tests. It records **obvious unit-test misses** and **integration scenarios that can move (fully or partially) to unit level** to enable earlier failure. No implementation is included.

## Goals

- Identify high-value, feasible host unit-test additions (logic, parsing, state machines, error/edge handling).
- Identify integration cases whose assertions duplicate or can be front-loaded by unit tests, while preserving necessary end-to-end coverage.
- Provide a prioritized, incremental path that fits existing test infrastructure (`test/mocks`, Unity, frontend Vitest/TS tooling where relevant).

## Non-goals

- Removing integration tests that validate real platform behavior (LittleFS/NVS on target, boot sequencing, UART/shell REPL wiring, QEMU/runtime constraints) without equivalent confidence.
- Large refactors solely for testability; where seams are required they are called out as enablers (aligns with R25 #4/#11 in [`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md)).
- Hardware-only validation (touch/network coexistence/perf) beyond noting boundaries.
- Frontend/compiler parity work beyond noting it as adjacent coverage (tracked under R25 #5/#11b/#14).

## Current test landscape (evidence snapshot)

- **Host unit tests:** 24 C/C++ test executables defined in `test/CMakeLists.txt` (e.g. `test_vfs_path`, `test_shell*`, `test_http_mime/auth/upload`, `test_network_url`, `test_calibration*`, `test_time_sync`, `test_bluetooth_*`, `test_text_buffer`, `test_ui`, `test_berry_source_buffer`). Rich ESP-IDF/driver/RTOS mocks exist under `test/mocks/`.
- **Frontend tests:** Vitest suites under `frontend/src/**` and `frontend/tools/compile-storage.test.ts` (compiler/IDE/storage-tooling coverage).
- **Integration suites:** `esp_littlefs` (migrated Unity suite), `vfs_ops` (Unity on target FS), `nvs_calibration` (Unity on target NVS), `init_sequence` (boot log ordering/markers), `system` (REPL commands over serial), `shell_fs` (broad shell/FS/alias/berry-script behavior over REPL), `stability` (longer-running checks) — driven by `integration_test/pytest.ini` and per-suite `test_*.py`.

## Obvious unit test misses (host-level)

Prioritized by risk/ROI and feasibility with existing mocks/patterns.

| Priority | Area | Current state | Why it matters | Suggested unit coverage (examples) | Notes / seams |
|---|---|---|---|---|---|
| P1 | HTTP API handlers/helpers (`components/http_server/src/http_api.c`) | No host tests; only `http_mime`, `http_auth`, `http_upload`/path helpers covered (`test_http_mime.c`, `test_http_auth.c`, `test_http_upload.c`). | Large surface, manual JSON/path/query/status handling (duplication/escape/error-mapping risk per R25 #3). | Pure/helper extraction tests: path/query decode/validate, VFS→HTTP error mapping, JSON escaping/writer fragments, response shaping for representative endpoints (list/read/write/remove/mkdir cases) using `mock_httpd`/`mock_filesystem`. | May require small refactors to isolate handlers from `esp_http_server` glue; start with extracted helpers. |
| P1 | HTTP multipart/static edge cases (`http_multipart.c`, `http_static.c`, `http_path.c` beyond current sanitize cases) | `http_path` sanitization tested indirectly via `test_http_upload`; multipart/static behavior largely untested at host level. | Upload/streaming and static serving bugs are costly and often logic-level (boundary parsing, headers, path safety). | Boundary/header parsing, chunk/state transitions, path resolution rules, content-type/static lookup decisions with table-driven cases. | Reuse `mock_httpd` request/response patterns where possible. |
| P1 | VFS high-level API behavior (`components/filesystem/src/vfs.c`, coordination with `flash.c`/path rules) | `vfs_path` unit-tested; real read/write/list/mkdir/remove/exists/is_dir semantics covered in `integration_test/test_vfs_ops/main/test_vfs_ops.c` on LittleFS. | Argument validation, path normalization, error propagation, and API contracts can fail early without real flash. | Contract tests with mocked filesystem/flash backends: null/invalid args, error mapping, directory/file distinctions, overwrite/empty-file/list behavior expectations. | Requires/increases mock seam for underlying FS calls; complements (not replaces) LittleFS integration. |
| P1 | Shell FS command semantics (`components/shell/src/shell_fs_cmds.c`, redirection/quoting interactions as applicable) | `test_shell.c`/`test_shell_script.c` cover core shell behavior; many file-operation behaviors asserted only in `integration_test/test_shell_fs` REPL flows. | `ls/cat/touch/mkdir/cd/pwd/rm/rmdir/cp/mv/hexdump/info` logic, messages, and edge cases are deterministic given a filesystem backend. | Host tests invoking command handlers against `mock_filesystem` for success/error paths (nonexistent targets, dir-not-empty, overwrite/copy rules, relative resolution integration with `vfs_path`). | Keep REPL/autoexec/linenoise wiring in integration. |
| P2 | Network core/URL/HTTP helper logic (`components/network/src/network.c`, `network_status.c`, `network_http.c`, `network_url.c`) | `network_url` has unit tests (`test_network_url.c`); HTTP client/state machine and status behavior lack host coverage. | `network_http.c` is a concentrated single-flight/stateful path (timeouts/abandon/buffers) noted as hotspot (R25 #6). | Expand URL edge cases; add host tests for request validation, header/body assembly helpers, timeout/argument bounds, and state transitions where separable from `esp_http_client`. | Full client behavior may remain integration/manual; extract/test pure helpers first. |
| P2 | WebSocket component (`components/websocket/src/websocket.c`) | No host tests found. | Protocol/framing/state handling regressions are easier to catch with deterministic inputs. | Frame/handshake/message parsing & state helpers with mocked transport/server hooks. | Depends on existing/new seams around `esp_http_server`/ws APIs. |
| P2 | Time sync edge behavior (`components/time_sync/src/time_sync.c`, `time_sync_cmd.c`) | `test_time_sync.c` exists but scope should be reviewed against SNTP/state/parse edges. | Timezone/NTP config, status transitions, and retry/failure reporting are logic-heavy. | Expand unit cases for invalid config, state transitions, formatting/command output, callback-driven updates via `mock_sntp`/timer mocks. | Real SNTP/network timing remains integration/system-level. |
| P2 | Berry runtime/script integration surfaces (`components/berry_lang/src/*` bindings, module/command bridges) | `test_berry_source_buffer.cpp` and shell/berry command tests exist; integration covers `berry run` scripts exercising `fs`/`net`/`console` APIs (`test_shell_fs`). | Binding contract/arg validation and simple API semantics can be pinned cheaply at host level. | Targeted host tests for binding adapters/helpers and script-loading/resolver contracts where mocks permit; keep VM/runtime-on-target smoke in integration. | `berry_runtime` is a known hotspot (R25 #1/#4); prioritize seams before broad tests. |
| P3 | Init/composition helpers | `init_sequence` integration asserts boot markers/order. | Pure ordering/config decisions (when extracted) are unit-testable. | If startup is refactored into phases/tables (R25 #2), add unit tests for phase selection/order decisions independent of hardware init. | Not feasible/useful without extraction; keep current end-to-end check. |

## Integration scenarios movable to unit level (earlier failure)

These are candidates to **add or strengthen unit tests for first**, so failures occur before QEMU/flash/NVS/REPL runs. Integration tests should remain for behaviors that require the real platform.

| Integration area | Evidence (current) | Unit-testable now / with small seams | Keep in integration because |
|---|---|---|---|
| VFS CRUD, errors, metadata (`test_vfs_ops`) | `integration_test/test_vfs_ops/main/test_vfs_ops.c` + `test_vfs_ops.py` (Unity over real mounted flash). | API arg validation, path/error contract, list/exists/dir semantics via host tests over mocked/seamed backend (and existing `vfs_path` tests). | Real LittleFS behavior, wear/format/mount, flash accounting nuances (`grow.bin` note), partition/config integration. |
| NVS calibration persistence (`test_nvs_calibration`) | `integration_test/test_nvs_calibration/main/test_nvs_cal.c` mirrors save/load/exists/validation on real NVS. | Storage logic/validation already partially host-testable (`test_calibration_storage.c`, `mock_nvs`); extend to cover same contract matrix if gaps exist. | Real NVS init/erase/versioning behavior and flash partition interaction. |
| Shell filesystem commands & messages (`test_shell_fs` FS/alias/error cases) | `integration_test/test_shell_fs/test_shell_fs.py` asserts `ls/cat/touch/mkdir/cd/pwd/rm/rmdir/cp/mv/hexdump/info`, aliases, and error strings over REPL. | Command parsing/dispatch outcomes and filesystem side effects with `shell_logic` + `mock_filesystem`/`mock_console` (many cases deterministic). | End-to-end REPL, prompt/line editing, UART/console transport, mounted LittleFS, actual command registration/startup environment. |
| Shell help/autocomplete/script resolution | `help`, `ber<TAB>`, path completion, `autoexec` behavior in `test_shell_fs`. | Resolver/completion helpers and command registration surfaces (`test_shell_script.c`, completion units if present/seamable). | Linenoise/terminal interaction, real filesystem contents, boot-time autoexec wiring. |
| Berry `fs`/`net`/`console` script contracts | `berry run /flash/*.be` scripts in `test_shell_fs` check API return shapes/errors/async input markers. | Argument/bounds/error mapping and simple helper behavior in native bindings with host-side Berry/mocks where practical. | VM execution on device runtime, module loading from flash, scheduler/console async behavior, memory/runtime constraints. |
| Network-unavailable command behavior (`ping`, `nslookup`) | `test_shell_fs` expects `ESP_ERR_INVALID_STATE` via REPL. | Command argument parsing, timeout bounds, and error-to-message formatting with mocked `net.ping`/network layer. | Actual network stack state, DNS/ping integration on target/Wi-Fi environments. |
| System info commands (`test_system`) | `uptime`, `memory` over REPL. | Formatting/parsing of values from injected system/heap providers (`mock_system`/heap hooks) where useful. | Real FreeRTOS tick/heap reporting and console task wiring. |
| Boot/init sequencing (`test_init_sequence`) | Log/marker ordering: NVS, FS, system, shell, `INIT_OK`. | Only if decomposed into explicit pure init-phase decisions (future). | Genuine component initialization order/side effects on target. |
| esp_littlefs upstream/migrated behaviors | `test_esp_littlefs` runs migrated Unity suite on target. | n/a for filesystem driver semantics. | Validates the actual LittleFS port/config/flash characteristics used by firmware. |

## Suggested execution order (incremental)

Aligned with R25 execution order ([`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md): #11a/b/d as quick wins, #4/#6 as seam/stabilization enablers):

1. **Quick wins, no/low refactor:** extend host tests for existing seams — HTTP path/multipart/static helpers, `network_url` edges, `time_sync` edges, shell FS command cases via `mock_filesystem`, and any missing `calibration_storage` contract cases mirrored from integration (R25 #11a/#11d).
2. **Extract-and-test helpers:** factor JSON/path/error/response helpers out of `http_api.c` (R25 #3) and pure request/state helpers out of `network_http.c` (R25 #6) with unit tests.
3. **Add testability seams:** introduce/increase injectable interfaces for VFS backend, websocket transport, and Berry binding/runtime touchpoints (R25 #4) to move more contract checks host-side.
4. **Rebalance integration suite:** after unit coverage lands, trim or keep only high-value end-to-end assertions in integration tests (real mount/init/REPL/runtime) to reduce duplication and runtime while preserving confidence.
5. **Process guardrails:** when adding logic to currently under-tested areas, require accompanying host tests in the same change (and run `make test` per `AGENTS.md`).

## Acceptance criteria (for promotion to implementation work)

- [ ] Findings triaged into consolidated plan items/follow-ups with owners and scope, cross-referenced with R25 ([`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md)) where items overlap (#3/#4/#6/#11).
- [ ] P1 unit-test additions implemented for selected areas (at least HTTP helper/API seams, VFS/shell command contracts, and multipart/static helpers as feasible).
- [ ] Integration tests reviewed: duplicated logic assertions covered by host tests, with end-to-end/platform coverage retained (LittleFS/NVS/boot/REPL/runtime boundaries preserved).
- [ ] `make format-check` and `make test` pass for host-side changes; affected integration suites built/run when their scope changes (per `AGENTS.md`).
- [ ] Evidence/coverage notes linked from the consolidated plan and, when relevant, from R25 progress.

## Open questions

- How much of `network_http.c` can be made host-testable without distorting production structure given ESP-IDF client coupling?
- Should shell command integration tests keep message-string contracts verbatim, or stabilize on structured/behavioral assertions to ease refactoring?
- Which Berry binding contracts are stable enough to pin in host tests vs. better validated as device smoke tests?
