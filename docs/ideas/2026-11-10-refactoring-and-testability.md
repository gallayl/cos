# R25 — Codebase refactoring, architecture hardening, and testability enablement

- **Priority:** P1 (staged; see priority table) | **Complexity:** L (program of work) | **ROI:** High | **Risk:** Med–High (hot paths on constrained ESP32)
- **Source:** Whole-repo analysis (`main/`, `components/`, `frontend/`, `storage/ts` + `storage/flash_data`, `test/`, `integration_test/`, build/scripts/docs)
- **Date:** 2026-11-10
- **Status:** Idea / proposal (not yet committed to consolidated plan)
- **Related:** [Unit test coverage gaps and integration-to-unit rebalancing](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md) (detailed inventory for #11 and integration→unit candidates)

## Summary

Persisted analysis findings identifying refactor opportunities, coding-practice issues (SOLID, DRY, KISS, YAGNI), and unit/integration test coverage enablement gaps. No code changes are proposed as part of this document; items should be promoted deliberately into `docs/plan/consolidated-improvement-plan.md` when scheduled.

For a concrete, evidence-based breakdown of **host unit-test misses** and **integration scenarios that can be front-loaded to unit level** (with priorities, seams, and "keep in integration" boundaries), see [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md). That plan complements this document: #3/#4/#6 focus on refactors/seams, while #11 and the plan focus on coverage and test-boundary rebalancing.

**Notable context**
- No actionable `TODO`/`FIXME`/`HACK` markers found in source during analysis (only a non-actionable reserved define noted in `scripts/berry_conf.host_validation.h`).
- Repo hygiene: tracked build artifacts exist under `integration_test/test_shell_fs/build/**` (and related outputs). Cleaning them is low runtime risk but can affect CI assumptions; treat as a separate hygiene task.

## Notable large / complex hotspots

| Area | File | Approx. size | Why notable |
|---|---|---:|---|
| Berry runtime | `components/berry_lang/src/berry_runtime.cpp` | ~65 KB | God module: VM lifecycle/init-deinit, GC policy/telemetry, script/app state, console I/O bridge, native module registration, UI app lifecycle, mutex/heap/log guards, broad dependencies. |
| Bluetooth | `components/bluetooth/src/bluetooth.c` | ~23 KB | Large stateful integration surface (plus `bluetooth_hid.c`). |
| Shell input | `components/shell/src/shell_input.c` | ~23 KB | Concentrated input/line-editing/prompt/history concerns. |
| Berry networking | `components/berry_lang/src/berry_network.cpp` | ~23 KB | Script bindings coupled with network behavior. |
| Text console | `components/text_console/src/text_console.cpp` | ~22 KB | Rendering, stdout/log hooks, VT100/state, visibility/history (paired with `text_buffer.c`). |
| HTTP client | `components/network/src/network_http.c` | ~20 KB | Single-flight worker, static request/response buffers, timeout/abandon semantics. |
| Network URL | `components/network/src/network_url.c` | ~18 KB | Parsing/composition tightly related to fetch API surface. |
| Frontend compiler | `frontend/src/ide/berry/statement-generator.ts`, `expression-generator.ts`, `code-generator.ts`, `frontend/tools/compile-storage.ts` | ~61 KB combined | TS AST → Berry lowering and storage build/check pipeline. |
| Frontend IDE/editor | `frontend/src/editor/file-tree-view.tsx`, `editor-app.tsx`, `frontend/src/editor/file-tree.ts` | ~44 KB combined | UI state + filesystem/editor behaviors concentrated. |
| Host tests/build | `test/test_text_buffer.c`, `test/test_shell.c`, `test/test_shell_input.c`, `test/CMakeLists.txt` | large | Complex units/mocks; test build config is a maintenance hotspot. |

## Prioritized findings

### Priority 1 — High impact, best ROI / enables safer change

| # | Principles | Finding | Why it matters | Suggested refactor | Key files | Impact | Risk | Effort |
|---|---|---|---|---|---|---:|---:|---:|
| 1 | SRP, KISS, testability | **`berry_runtime` is a god module** mixing VM exec/GC, app/script state, console bridge, native/module registration, UI app lifecycle, diagnostics/guards with broad `REQUIRES`. | Largest/hottest surface; hard to reason about, extend, and test in isolation. | Decompose into focused units (VM/core exec & GC, app lifecycle/UI launch, console bridge, module/native registry, diagnostics/guards); keep `berry_runtime.h` stable initially via adapters/wrappers. | `components/berry_lang/src/berry_runtime.cpp`, `berry_ui.cpp`, `berry_network.cpp`, `be_modtab.c`, `be_port.c`, `components/berry_lang/include/berry_runtime.h`, `src/berry_internal.h`, `CMakeLists.txt` | High | High | L |
| 2 | DIP, OCP, testability | **Manual, order-sensitive composition root** in `app_main` (hard-coded init/wiring for 17+ subsystems and callbacks). | Implicit dependencies make startup fragile; init slices hard to test. | Introduce explicit startup phases/init table or dependency-aware registration (named phases/helpers) with behavior preserved. | `main/main.c` (+ optional `main/init_*.c`) | High | Med | M |
| 3 | DRY, correctness | **HTTP API handler boilerplate and ad-hoc JSON** (repeated query/path extract+validate, status/body formatting, VFS→HTTP mapping, manual JSON escaping/building). | Duplication risks inconsistent behavior/escaping and error-prone new endpoints. | Extract shared helpers: API path parsing (decode/sanitize/resolve), JSON/string writer & escaping, `send_json_ok`/`send_api_error`, chunked response helpers. | `components/http_server/src/http_api.c` (align with `http_static.c`, `http_upload.c`, `http_path.c`, `http_multipart.c` as applicable) | Med-High | Low | S–M |
| 4 | SOLID/DIP, testability | **Global/static runtime state blocks isolation** in `berry_runtime` and `ui` (singleton root/task/flags/callbacks), plus RTOS/hardware coupling. | Order-independent host tests are difficult for core/runtime paths. | Add explicit reset/test hooks and injectable seams for side effects (clock/timer, heap/stats, console transport, task/sync adapters) starting with Berry/UI hot paths. | `components/berry_lang/src/berry_runtime.cpp`, `include/berry_runtime.h`, `components/ui/src/ui.cpp`, `include/ui.h`, selected hotspots (`network_http.c`, `text_console/*`) | High | Med | M |
| 5 | DRY, process correctness | **Parallel `storage/ts` and `storage/flash_data` (`.be`) trees** can drift despite `compile-storage` tooling/checks. | Device/script mismatch risk if `.be` edited directly or checks not enforced (known handwritten exceptions). | Treat TS as single source in normal workflow: enforce compile/verify checks in CI/build, make `--write`/exceptions explicit, guard unintended hand edits. | `frontend/tools/compile-storage.ts`, `storage/ts/**`, `storage/flash_data/**`, build/CI scripts | Med | Low | S–M |

### Priority 2 — Medium impact, structural improvements

| # | Principles | Finding | Why it matters | Suggested refactor | Key files | Impact | Risk | Effort |
|---|---|---|---|---|---|---:|---:|---:|
| 6 | SRP, KISS | **`network_http.c` concentrates a complex single-flight protocol/state machine** (static buffers, worker task, sync primitives, timeout/abandon/completion, stack sampling). | Hard to read/extend/debug despite necessary safeguards. | Split internally into protocol/state, request/response handling, and esp_http_client/event helpers (behavior-preserving). | `components/network/src/network_http.c`, related `network*.c`/`network.h` | Med-High | Med | M |
| 7 | SRP, coupling | **UI uses hidden global control flow** (singleton root, action queue/redraw/exit flags, persistent task, execution/lock/tick callbacks). | Implicit lifecycle/scheduling; testing and evolution harder. | Introduce UI runtime/context object owning state/task/callbacks; keep existing `ui::`/C/Berry bindings as adapters. | `components/ui/src/ui.cpp`, `components/ui/include/ui.h`, `components/berry_lang/src/berry_ui.cpp` | Med-High | Med-High | M–L |
| 8 | ISP, SRP | **`network` API conflates concerns** (ping/DNS, URL/base, HTTP fetch, Wi-Fi/host info) in one header/surface. | Broad interface increases coupling; extension harder. | Segregate by concern with compatibility wrappers; clarify ownership boundaries. | `components/network/include/network.h`, `components/network/src/network.c`, `network_url.c`, `network_http.c`, `network_status.c` | Med | Med | M |
| 9 | DRY, contract drift | **Firmware bindings vs frontend COS SDK/types maintained manually**. | IDE types and device behavior can diverge. | Shared contract/source-of-truth (generate or validate `.d.ts`/inventory against Berry/native registrations and C APIs). | `frontend/src/cos-sdk/**`, `frontend/src/ide/cos-binding-inventory.ts`, `cos-sdk-registration.ts`, `components/berry_lang/src/berry_runtime.cpp`, `berry_ui.cpp`, `berry_network.cpp` | Med-High | Med | M |
| 10 | KISS | **Diagnostics/policy interleaved with core execution** (heap/GC telemetry/guards/checkpoints in Berry runtime). | Obscures primary control flow in critical code. | Extract diagnostics/guard/telemetry helpers behind small interfaces/helpers; preserve behavior/logging. | `components/berry_lang/src/berry_runtime.cpp` | Med | Med | M |
| 11 | Coverage enablement | **Thin/absent host coverage in areas**: HTTP API/static/multipart/path end-to-end behavior, websocket, time_sync edges, filesystem VFS ops beyond path helpers, Berry/network bindings, shell cmd modules as units; integration breadth narrow (Wi-Fi/HTTP API/BT-HID/websocket/Berry lifecycle under pressure). | Regressions in integration paths can slip past current tests/mocks. | (a) Add host tests for extracted HTTP helpers/handlers and `network_url` edge cases. (b) Strengthen generator/storage parity tests for tricky TS→Berry cases. (c) Add targeted QEMU/integration scenarios where feasible (websocket bridge, basic HTTP API CRUD, Berry app launch/close memory smoke). (d) Rebalance integration→unit where logic/contracts are deterministic; see detailed inventory in [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md). | `test/test_http_*`, `test/test_network_url.c`, `test/CMakeLists.txt`, `frontend/src/ide/ast-to-berry.test.ts`, `frontend/tools/compile-storage.ts`, `integration_test/*` | Med-High | Low–Med | S–M / M–L |

### Priority 3 — Lower urgency / incremental or strategic

| # | Principles | Finding | Why it matters | Suggested refactor | Key files | Impact | Risk | Effort |
|---|---|---|---|---|---|---:|---:|---:|
| 12 | ISP, cohesion | **`display.h` is a broad C facade**; `text_console` API spans rendering/logging/persistence/VT100/hooks. | Consumers depend on more than needed; narrow fakes harder. | Incrementally split/narrow interfaces (primitives vs text/console helpers) and reduce cross-cutting responsibilities. | `components/display/include/display.h`, `components/text_console/include/text_console.h`, `components/text_console/src/*` | Med | Low–Med | M |
| 13 | YAGNI, KISS | **Possible unused/legacy exports/wrappers** (not exhaustively proven; no TODO debt found). | Extra surface area increases maintenance/test burden. | Audit with build/coverage/call-site analysis; remove only with confirmation. | `components/berry_lang/include/berry_runtime.h`, `components/network/include/network.h`, UI/shell APIs | Low–Med | Med | S–M |
| 14 | DRY (frontend compiler) | **Large, pattern-heavy TS→Berry lowering modules** with parallel cases. | Maintenance cost and risk of inconsistent lowering as support grows. | Consolidate shared emit/visitor/util patterns backed by strong snapshot/fixture coverage. | `frontend/src/ide/berry/statement-generator.ts`, `expression-generator.ts`, `code-generator.ts`, `class-generator.ts`, `frontend/src/ide/ast-to-berry.test.ts` | Med | Med | M |
| 15 | Repo hygiene | **Tracked build artifacts in integration tests** (e.g. `integration_test/test_shell_fs/build/**`). | Repo bloat, noisy diffs, can interfere with structural moves. | Remove with CI alignment and `.gitignore` updates (verify assumptions first). | `integration_test/test_shell_fs/**`, related build outputs | Med (maintainability) | Low–Med | S–M |

## Cross-cutting themes

- **Architecture/SOLID:** god modules and hidden global control flow (`berry_runtime`, `ui`), manual composition (`main.c`), broad interfaces (`network.h`, `display.h`, `text_console.h`).
- **Duplication/DRY:** HTTP API response/path/JSON handling; TS source vs generated `.be` parity; firmware bindings vs frontend COS SDK/types; repeated lowering patterns in compiler.
- **Complexity/KISS/YAGNI:** concentrated state machines and interleaved diagnostics in hot paths; potential unused surface (verify before removal).
- **Testability/coverage:** global state and hardware/RTOS coupling limit isolation; rich mocks exist but seams/reset hooks needed in hotspots; gaps in HTTP API/handler, websocket, time_sync, VFS ops, Berry/network bindings, and integration scenarios. See also the coverage/integration-rebalancing inventory in [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md) (obvious unit misses, integration→unit candidates, and platform-boundary guidance).

## Suggested execution order

1. **Quick, low-risk wins:** #3, #5 and host-test additions (#11a/b, #11d) to lock behavior before deeper changes (prioritized cases and integration→unit mapping in [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md)).
2. **Stabilize core hotspots:** #4 (seams/reset hooks) alongside incremental #1 decomposition (behind stable APIs) and #6.
3. **Architecture/coupling cleanup:** #2, then #7/#8/#9/#10.
4. **Strategic/hygiene:** #12–#15 as capacity allows.

## Acceptance criteria (for promotion to plan)

- [ ] Items triaged and mapped to consolidated plan entries (new R-items or follow-ups) with owners/scope.
- [ ] For each executed refactor: behavior-preserving where stated, unit/integration tests added or updated, and docs updated as needed.
- [ ] Startup sequencing, Berry/UI lifecycle, and network HTTP timeout/single-flight semantics validated on host tests and (where relevant) QEMU/hardware.
- [ ] Storage generation/parity checks enforced in CI for in-scope script paths.
- [ ] Validation evidence linked from the consolidated plan.

## Progress

- [x] Analysis performed and documented (2026-11-10)
- [x] Detailed coverage/integration-rebalancing inventory documented — see [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md) (2026-11-14)
- [ ] Triaged into consolidated improvement plan
- [ ] Quick wins implemented (#3, #5)
- [ ] Testability seams/reset hooks implemented (#4)
- [ ] Structural decompositions in progress (#1, #6, #7)
- [ ] Validation evidence collected
