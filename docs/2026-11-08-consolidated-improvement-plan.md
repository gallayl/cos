# COS Consolidated Improvement Plan

**Created:** 2026-11-08  
**Scope:** Consolidates `docs/2026-10-improvements.md` and `docs/2026-11-01`–`2026-11-07`  
**Purpose:** Single progress-tracked plan for remaining work, with priority, order, findings, specs/acceptance references, and follow-ups.

## How to use this document

- Treat this as the **cross-cutting tracker**. Source documents remain the detailed design/spec history.
- Update status checkboxes and the **Progress log** when work lands.
- Do not mark an item complete without validation evidence (tests, CI, `idf.py size`/heap measurements, or hardware validation where required).
- Prefer small, independently testable changes. For memory/runtime work, **measure before and after**.

## Findings (2026-11-08 review)

### Already delivered / largely complete

| Area | Evidence | Notes |
|---|---|---|
| Shell/display/CI milestones 1–6 | `docs/2026-10-improvements.md` validation logs; PRs #14–#18 referenced there | Milestone 4 has **pending hardware validation** for touch scrollback gestures and connected-network `ping`/`nslookup`. |
| HTTP/network buffer consolidation (P1) | Commit `e0bf05b`; `components/network/src/network_http.c`, `components/network/include/network.h`, `components/berry_lang/src/berry_network.cpp` | Removed redundant idle response/body/header storage patterns; added `network_http_request_header_t` request view; worker copies request data into network-owned buffers before queueing. |
| Lazy JSON/module hygiene (R1/R6) | `berry_runtime.cpp`: `berry_runtime_ensure_json()`, lazy proxy globals for `console/fs/net/ui`, `json` on-demand | R1 removed eager `json` preload (hardware validated). R6 removed eager `console/fs/net/ui` preload and added lazy proxy globals for bare-global compatibility; `cos_import`/Berry `import` load on demand. **Hardware validation pending for R6 heap/behavior.** |
| Frontend TS/Berry IDE foundation | `frontend/` (Shades + Vite + Monaco), `frontend/src/cos-sdk/*`, `frontend/src/ide/*`, `frontend/tools/compile-storage.ts` | TS→Berry compiler, diagnostics, tests, relative + `@cos/*`/`@berry/*` `cos_import` lowering, type-only import stripping. |
| TS import features (compiler side) | Commits `1c945ae`–`3659f62`; `frontend/src/ide/berry/statement-generator.ts`, `ast-to-berry.test.ts` | Type imports done; relative/custom imports and `@cos/*`/`@berry/*` module specifiers supported in compiler. SDK module declarations exist. |

### Open / partially open

| Area | Source docs | Current gap |
|---|---|---|
| Memory pressure follow-ups | `2026-11-02`, `2026-11-06` | R7 (`include_body`) implemented and hardware validated (2026-11-08); P4 stack measurement/tuning in progress (R8); P5 TLS peak measurement; R2 follow-ups (HTTPS/timeout cleanup/BT coexistence/regression completion); R5 follow-ups (stdio/heap corruption, heap reporting). |
| Firmware module loading hygiene | `2026-11-06` P2, `2026-11-07` | R1 made `json` on-demand; R6 implemented lazy proxy/on-demand loading for `console/fs/net/ui` (hardware validation pending). |
| TS compiler/runtime semantics | `2026-11-03`, `2026-11-07` | R3 delivered `.length`→`size()`/numeric/diagnostic conformance pass and hardware validation. R4 complete: `try`/`catch`/`throw` compiler lowering implemented, unit tested, and hardware validated (`finally`/destructuring catch diagnosed). `async`/`await`/`for await` still rejected; broader subset/semantic docs/tests remain incomplete. |
| Berry shell command scripts | `2026-11-01` | No resolver, persisted `path` command, `main(args)` command-script execution, or migration pilot wiring found. |
| TS toolchain productization | `2026-11-03`, `2026-11-04` | Package layout differs from proposal; Worker isolation for compilation must be verified/enforced; SDK single-source-of-truth/audit missing; dev/prod profiles, source maps, editor memory diagnostics missing. |
| Berry simulator | `2026-11-05` | No WASM runtime, simulator Worker, platform abstraction backend, renderer, or simulator UI found. |
| Deferred shell/UX items | `2026-10` | Pipes/redirection design, shell line-editor refactor seam, larger apps/editor/calendar, image preview feasibility. |

## Priority and ordering

**Priority levels**
- **P0:** Correctness, safety, memory/startup pressure, or measurement needed before risky changes.
- **P1:** High-value developer/runtime capability with bounded scope.
- **P2:** Important but larger/experimental/riskier; requires design or benchmarks.
- **P3:** Strategic or backlog items.

**Suggested execution order** (dependency-aware): R1 → R2 → R3 → R4 → R5 → R6 → R7 → R8 → R9 → R10 → R11 → R12 → R13 → R14+.

## Overall progress tracker

| ID | Item | Priority | Status | Target milestone |
|---|---|---:|---|---|
| R1 | Finish lazy JSON / startup module hygiene | P0 | ✅ Done | M7a |
| R2 | 2026-11-06 profiling + regression matrix (P7) | P0 | ✅ Done (follow-ups tracked) | M7a |
| R3 | TS→Berry semantic conformance pass | P0 | ✅ Done | M7b |
| R4 | `try`/`catch`/`throw` compiler lowering | P1 | ✅ Done | M7b |
| R5 | Phase 1 Berry memory instrumentation & lifecycle verification | P0 | ✅ Done (R5-FU1/FU2 follow-ups open) | M7c |
| R6 | Firmware on-demand native module loading | P1 | ✅ Done | M7c |
| R7 | Fetch `include_body` option (P3) | P1 | ✅ Done | M7d |
| R8 | HTTP worker stack/TLS measurement & safe tuning (P4/P5) | P1 | ✅ Done (stack kept 6144; P5/`-141` heap follow-up open) | M7d |
| R9 | Berry command-script resolver + `path` + `ping` pilot | P1 | ✅ Done (native `ping` migration pending decision) | M8 |
| R10 | Frontend TS toolchain hardening (Worker deferred; layout/SDK registration/device API) | P1 | ✅ Done (Worker deferred) | M8 |
| R11 | SDK single-source-of-truth / binding audit | P1 | ✅ Done (pragmatic inventory + automated audit) | M8 |
| R12 | Milestone 4 hardware validation (touch/network) | P1 | ⬜ Pending hardware | M8 |
| R13 | Memory-conscious compiler rules + resource lifecycle contract | P2 | ⬜ Not started | M9 |
| R14 | Compiler profiles, debug metadata, source maps, editor diagnostics | P2 | ⬜ Not started | M9 |
| R15 | Optional `.bec` bytecode pipeline/loader | P2 | ⬜ Not started | M9+ |
| R16 | Native UI allocation benchmarking/policy changes | P2 | ⬜ Not started | M9+ |
| R17 | Berry solidification evaluation (firmware-bundled only) | P2 | ⬜ Not started | M9+ |
| R18 | `async`/`await` runtime design + implementation | P2 | ⬜ Not started | M10 |
| R19 | In-browser COS simulator (Berry WASM) | P3 | ⬜ Not started | M10 |
| R20 | 2026-10 deferred backlog (pipes/redirection, editor seam, larger apps, image support) | P3 | ⬜ Backlog | Backlog |
| R21 | Unify `berry run`/`berry exec` on `main(args)` contract + explicit app launch | P2 | ⬜ Not started | M9 |

## Detailed plan

### R1 — Finish lazy JSON / startup module hygiene
- **Priority:** P0 | **Complexity:** S–M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P2, `docs/2026-11-07-ts-feature-extensions.md`
- **Problem:** `berry_runtime_ensure_json()` exists, but `berry_runtime_init()` still eagerly installs/loads `json` (and other module globals). This prevents the intended startup memory win and conflicts with on-demand module goals.
- **Spec / acceptance criteria:**
  - Ordinary Berry runtime startup does not load the JSON module.
  - `fs.read_json()` / `fs.write_json()` and `net.fetch(..., response_type="json")` still work.
  - Repeated JSON operations do not reload the module unnecessarily.
  - VM destroy/recreate resets JSON cache state correctly.
  - Existing `.be` apps that do not use JSON behave unchanged.
- **Implementation notes:** Verify whether `install_module_global`/`be_getmodule` eagerly loads; prefer explicit lazy registration/loading path consistent with Berry module semantics. Add focused unit/integration coverage for non-JSON and JSON flows.
- **Progress:**
  - [x] Design chosen and documented in PR/commit message. (Remove eager `install_module_global("json")` from `berry_runtime_init()`; keep `json` in native module table for `import`/`be_getmodule`; rely on existing `berry_runtime_ensure_json()` used by FS JSON and `net.fetch(..., response_type="json")` to load+cache the module/global on first use. Bare global `json` before first ensured load is no longer preloaded, but existing `.be` apps in `storage/flash_data` do not use bare `json`.)
  - [x] Code change implemented.
  - [x] Tests added/updated. (No host/unit Berry runtime harness in repo; validated via hardware/manual tests instead.)
  - [x] Heap/startup sanity check recorded (hardware). `mem` after boot/non-JSON run showed normal heap (`Free 18344/18492`, `Min 15084/9580`, `Max alloc 17408/16384`); post-JSON `Max alloc` dropped to `6400` as expected for on-demand JSON/module use. Formal before/after startup baseline deferred to R2 profiling matrix.

### R2 — 2026-11-06 profiling + regression matrix (P7)
- **Priority:** P0 | **Complexity:** M | **ROI:** High | **Risk:** Low–Medium
- **Status:** ✅ **Completed as tracker item (2026-11-08)** — R2 deliverables committed and hardware profiling surfaced actionable issues. Remaining validation/fixes are tracked as follow-ups below (primarily R8 and network/runtime hardening).
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P7
- **Spec / acceptance criteria:** Record measurements at the checkpoints defined in P7 (startup/before Berry init/after init/app open/FS text/FS JSON/HTTP GET/HTTPS GET/max response/after GC/repeated requests). Capture `free`, `largest block`, `minimum free`; compare `idf.py size`/`size-components` and `.bss` impact in `berry_lang`/`network`. Run required regression cases (JSON lazy behavior, FS text/JSON, HTTP/HTTPS, large body, max response, timeout safety, repeated requests, worker stack sanity).
- **Progress:**
  - [x] Measurement method/script/checklist committed. Added `docs/reports/2026-11-08-r2-profiling-checklist.md`, shell profiling commands (`berry profile`, `berry gc`), and deployable Berry regression scripts under `storage/flash_data/tests/r2/`.
  - [x] Baseline numbers recorded in linked report. Initial `idf.py size`/`size-components` and boot/runtime measurements recorded in `docs/reports/2026-11-08-r2-profiling-checklist.md`.
  - [~] Regression matrix executed (partial) — follow-up. FS text/JSON pass; HTTP success path functional but costly; HTTPS progressed from TLS alloc `-0x008D` to PSA cert-verify OOM `-141` after memory tuning (`sdkconfig.defaults` mbedTLS 4KB/variable buffers, `HTTP_TASK_STACK` 6144, smaller `libnetwork` buffers/client I/O). Timeout/error paths can still collapse native heap (`min` very low, BT alloc failures, post-timeout `esp-tls`/`HTTP_CLIENT` errors). Repeated successful HTTP, HTTPS success on valid endpoint with cert verification, large body, max response, and isolated clean-boot P7 checkpoints still need follow-up runs.
  - [x] Anomalies documented as follow-ups in report (formal external issue filing optional).
- **Follow-ups (not blocking R2 closure):**
  - [ ] **HTTPS memory (R8/P5, critical):** resolve PSA/X.509 verification `PSA_ERROR_INSUFFICIENT_MEMORY` (`-141`) while keeping certificate verification enabled. R8 evidence (2026-11-09): HTTPS succeeded in healthy states (`example.com`, `httpbingo.org/bytes/2000`), but `postman-echo.com` and later `example.com` failed with `-141` under heap pressure (clean-boot-adjacent: min fell to 1968/692; long-session: heap minima down to 84). No crash; GC recovered free heap. Validate on clean boot with BT state recorded; investigate BT coexistence RAM, mbedTLS 4.x/PSA/ESP-IDF 6.1 memory options compatible with verification, cert-bundle verification peak, request sequencing/policy, and safe peak-heap reductions. Draft: `docs/reports/2026-11-09-r8-followups-https-heap.md`.
  - [x] **Timeout/abandon cleanup (R8, hardware-validated in scope):** software safety fix landed; R8 hardware validated transport-failure stability and true caller-abandon path (`/delay/5`, 300/300ms → `ESP_ERR_TIMEOUT`, immediate follow-ups `ESP_ERR_INVALID_STATE` while abandoned worker busy; no crash/stale data). Remaining: optional post-abandon-completion retry capture; deeper `esp_http_client` cancellation/heap behavior only if a reproducible issue appears.
  - [ ] **Regression completion:** isolated clean-boot runs for P7 checkpoints #1/#3/#8/#10 (success path), repeated successful HTTP 200, HTTPS success on valid endpoint (when heap healthy), large body near `NETWORK_HTTP_BODY_MAX` (now 2048), max response, and timeout use-after-free/buffer-overwrite sanity.
  - [ ] **BT coexistence:** measure/decide product behavior when BLE HID/BT and HTTPS/networking run together; consider runtime release/disable strategy if required.

### R3 — TS→Berry semantic conformance pass
- **Priority:** P0 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-03-ts-sdk.md`, `docs/2026-11-07-ts-feature-extensions.md`
- **Problem:** The compiler implements useful lowering, but some mappings need verification against actual Berry semantics (`.length` currently lowers to `.size()` method; doc flags a possible mismatch). `null/undefined`→`nil`, equality/truthiness, template strings, class property init, and container/string APIs need a documented conformance matrix.
- **Spec / acceptance criteria:**
  - Every supported mapping has tests (compiler output +, where feasible, Berry/runtime behavior expectations).
  - Unsupported/ambiguous constructs produce diagnostics at original TS locations.
  - No silent semantic degradation.
  - SDK docs/types align with supported mappings.
- **Implementation notes (2026-11-08):**
  - Verified Berry semantics against pinned source (`components/berry_lang/CMakeLists.txt`, rev `65fce31732a9bdb91cb42cfc86e2b40f16c57514`, `BE_INTGER_TYPE=1` long, `BE_USE_SINGLE_FLOAT=1`): `size(x)` is the compatible global for strings/lists/maps/bytes; Berry strings do not generally expose `.size()` as a method. `/` performs C integer division when both operands are `int`; `int()`/`real()`/`number()`/`bool()`/`type()` are global builtins.
  - Compiler: `.length` now lowers to `size(expr)`. Numeric literal emission preserves source lexical form (`2.0`, `1e3`) with `_` separators removed. Added diagnostics for `int-literal / int-literal` (Berry int-division ambiguity), unary `+`, and clearer `typeof`/`void`/`delete` messages.
  - SDK: `language.d.ts` documents Berry numeric model, adds `int`/`real` type aliases and global `int`/`real`/`bool`/`size`/`type` declarations; `modules/math.d.ts` now declares common Berry math functions/constants.
  - Storage: regenerated `storage/flash_data/*.be` from `storage/ts/*.ts` via `frontend/tools/compile-storage.ts --write` (length/size mapping). `compile-storage` ignores hand-written Berry scripts under `storage/flash_data/tests/`.
  - Matrix/checklist: `docs/2026-11-08-ts-berry-semantic-conformance.md`, `docs/reports/2026-11-08-r3-hardware-checklist.md`.
- **Progress:**
  - [x] Mapping matrix written. (`docs/2026-11-08-ts-berry-semantic-conformance.md`)
  - [x] `.length`/`size()` behavior verified and fixed if needed. (Berry source/tests verified; compiler emits `size(expr)`.)
  - [x] Other semantic edge cases audited. (numeric literal preservation, int/int division diagnostic, unary +/typeof/void/delete diagnostics; null/undefined/equality/truthiness/template/class-init/import-export mappings documented)
  - [x] Tests/docs updated. (frontend tests: 82 passed / 16 files; storage TS→Berry outputs regenerated)
  - [x] Hardware/Berry VM runtime validation completed as possible by user (2026-11-08; see `docs/reports/2026-11-08-r3-hardware-checklist.md`).

### R4 — `try`/`catch`/`throw` compiler lowering
- **Priority:** P1 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Status:** ✅ **Done (2026-11-08)** — compiler lowering, diagnostics, frontend unit tests, and hardware/manual validation complete.
- **Source:** `docs/2026-11-07-ts-feature-extensions.md`
- **Current state:** `frontend/src/ide/berry/statement-generator.ts` lowers `throw` to Berry `raise` and `try/catch` to Berry `try`/`except ..`/`end` (with optional `as <identifier>` binding). `finally` and destructuring catch bindings are diagnosed (no silent miscompile). Berry semantics confirmed against pinned source tests (`build/berry-source/tests/exceptions.be`, `tests/class.be`, `tests/compiler.be`, `tests/division_by_zero.be`). User hardware validation (2026-11-08): `berry run /sdcard/tests/trycatch.be` produced expected output ending with `R4 OK` (basic catch, no-binding catch, rethrow, control flow/return in try, break in catch) with no uncaught exception/crash.
- **Spec / acceptance criteria:**
  - Standard TS `try/catch` lowers to Berry exception handling (`try`/`except`/`raise`) with predictable binding behavior.
  - `finally` scope decided explicitly: **diagnosed as unsupported** (Berry has no `finally`; emulation is control-flow-sensitive and risky). Restructure cleanup explicitly.
  - `throw` lowers to Berry-compatible exception raising.
  - Unsupported patterns are diagnosed, not silently miscompiled.
  - Generated Berry is readable and tested.
- **Berry mapping notes:**
  - `throw expr` → `raise <compiled expr>`.
  - `try { T } catch (e) { C }` → `try` / `T` / `except .. as e` / `C` / `end`. `e` receives Berry's first exception value (for many built-in errors this is the error type string; Berry also supports a second message parameter, not currently exposed from TS catch syntax).
  - `try { T } catch { C }` → `try` / `T` / `except ..` / `C` / `end`.
- **Progress:**
  - [x] Berry exception syntax/semantics confirmed for pinned Berry version (`65fce317...`, `raise`, `try/except [..|type] [as e[,m]]/end`).
  - [x] Lowering implemented (`frontend/src/ide/berry/statement-generator.ts`: `emitTry`, `emitThrow`).
  - [x] Diagnostics/tests added (frontend `ast-to-berry` R4 cases: bound/unbound catch, throw, rethrow/nesting, typed catch erasure, control flow in try/catch, `finally` diagnostic, destructuring catch diagnostic). `npx vitest run src/ide/ast-to-berry.test.ts` → 37 passed.
  - [x] SDK/docs updated (source docs + conformance doc + hardware checklist). Broader SDK error-model docs may follow if Berry exception message binding (second `except` arg) is needed.
  - [x] Hardware/manual validation completed (user, 2026-11-08): `docs/reports/2026-11-08-r4-hardware-checklist.md`.

### R5 — Phase 1 Berry memory instrumentation & lifecycle verification
- **Priority:** P0 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-02-memory-consumption-improvements.md`
- **Status:** ✅ **Done (2026-11-08).** Acceptance evidence recorded: checkpoint 1 (clean boot), checkpoints 2–7/teardown GC, source-buffer release + fragmented-heap fallback, R5-HW2 restart-loop regression (Tap Dash 10 + Breakout 7 cycles, no progressive leak), failure paths (missing/compile error/0-byte), and R5-HW3 GC benchmarks (200/150/125 identical workload, no measurable difference, no responsiveness impact noticed; **default remains 200, restored/confirmed**). Host validation/tests passed. Open follow-ups: **R5-FU1 (critical, stdio/heap corruption root cause)**, optional `memory_churn.be` redeploy/worst-case reload recheck. **R5-FU2 (heap reporting display): resolved (2026-11-09)** — code fix, cap-aware host tests (`test_system` 12/12, `test_shell_berry` 26/26 impacted run), and hardware verified (`berry profile`/`mem`/`memory_churn`/`berry gc`). Evidence: `docs/reports/2026-11-08-r5-hardware-results-partial.md`.
- **Spec / acceptance criteria:**
  - Lifecycle checkpoints report `be_gc_memcount()`, free internal heap, largest internal block, minimum free heap (watermark), and total free heap where useful.
  - `read_virtual_file()` / `execute_app_source()` release source buffers immediately after `be_loadbuffer()` (including compile failures), with ownership clarified against pinned Berry.
  - Repeated app start/stop tests detect leaks/fragmentation regressions.
  - GC step-rate experiments (200/150/125) benchmarked; default unchanged until evidence supports it.
  - Host-side CI target compiles all existing `.be` apps as a regression gate.
- **Progress:**
  - [x] Instrumentation added. `[R5]` lifecycle checkpoints in `berry_runtime.cpp` report `be_gc_memcount()`, default heap free/min/largest, and internal free/min/largest at VM init, source load, compilation, app construction, setup, teardown, and GC completion. Checkpoint 7 is a single consolidated log (`[R5] 7 GC complete | ...`, includes reclaimed bytes) to reduce stdio pressure.
  - [x] Runtime/profile API extended. `berry_runtime_memory_info_t` includes GC step rate and heap/internal snapshot; `berry profile` prints the extended snapshot; `berry_runtime_gc_collect()` and `berry gc` remain available.
  - [x] Source-buffer lifetime verified/fixed. `BerrySourceBuffer` + `compile_and_release_source` release source immediately after `be_loadbuffer()` (success/failure). `read_virtual_file()` stats first, allocates the short-lived source buffer, then reads via `vfs_read_file()` and uses robust fallback allocation (`malloc` / `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT` / `MALLOC_CAP_8BIT` / internal fallback) when default heap is fragmented. Buffer is freed after compile. Host helper test `test_berry_source_buffer` passes.
  - [x] Lifecycle cleanup hardened. `berry_ui_end_app()` guards VM callback cleanup when VM is null; `close_active_app()` teardown/GC flow unchanged and checkpointed.
  - [x] GC step-rate experiment support added. `berry gc-step <rate>` / `berry_runtime_set_gc_step_rate`; default remains **200**. **Benchmarks 200/150/125 pending hardware** — do not change production default without evidence.
  - [x] Repeatable regression assets added. `storage/flash_data/tests/r5/memory_churn.be` plus hardware restart-loop procedure in `docs/reports/2026-11-08-r5-hardware-validation-guide.md`.
  - [x] Host `.be` compile validation added. `scripts/validate-berry-scripts.ps1` compiles `storage/flash_data/**/*.be` (22 files) with COS global prelude; `scripts/berry_conf.host_validation.h` provided for COS-compatible host builds. Local run passed: **22/22** (`build/berry-host-cos/bin/berry.exe`).
  - [x] Host tests for changed interfaces passed (rebuilt in `test/build`): `test_shell_berry` **26/26**, `test_berry_source_buffer` **2/2**, `test_shell` **63/63**.
  - [x] Hardware validation complete (`docs/reports/2026-11-08-r5-hardware-results-partial.md`, 2026-11-08). Checkpoint 1 ✅; checkpoints 2–7/teardown GC ✅; R5-HW2 Tap Dash 10 + Breakout 7 cycles stable/no progressive leak ✅; failure paths ✅; R5-HW3 benchmarks 200/150/125 (identical workload, no measurable difference, no responsiveness impact noticed) ✅; **default GC step 200 restored/confirmed**. Note: on-device `memory_churn.be` was 0 bytes during HW3 (Breakout comparison valid). R5-FU1 not reproduced in latest capture.
- **Follow-ups:**
  - [x] **R5-HW1 (partial/validated in practice):** Checkpoint 1 captured on clean boot; Breakout 8223-byte source loads succeeded repeatedly in latest firmware (including post-close reloads in stable sample). Explicit worst-case `default largest < 8224` reload can be revalidated opportunistically if reproduced.
  - [x] **R5-HW2:** Restart-loop regression evidence recorded. Tap Dash 10 cycles + Breakout 7 cycles show stable post-warm-up Berry managed memory, stable watermarks/largest blocks, and no progressive leak. Evidence in `docs/reports/2026-11-08-r5-hardware-results-partial.md`.
  - [x] **R5-HW3:** Benchmarked 200/150/125 with identical workload (Breakout ×5 + churn ×3; churn file 0 bytes on device). All rates produced identical post-close/steady metrics (`managed=4551`, `reclaimed=10555–10581`, `heap_free=28400`, `heap_largest=8704`, `heap_min=2920`, `internal_min=26084`, `internal_largest=22528`); no responsiveness impact noticed. **Default remains 200; restored/confirmed.**
  - [ ] **R5-FU1 (critical):** Investigate `LoadStoreError` at `0x40000000` in stdio flush during R5 GC/checkpoint logging (`collect_berry_garbage` → `esp_log` → `vfprintf` → `stdout_write_hook` `text_console.cpp:210`). Reproduce with `logging error`; use heap poisoning/stack checks; inspect `text_console` stdout hook and memory ownership under fragmentation.
  - [x] **R5-FU2:** Fixed confusing heap reporting/display. `system_get_info()` reports default-heap fields via `MALLOC_CAP_DEFAULT` and internal via `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`; `berry_runtime_get_memory_info()` uses consistent default caps for `heap_free/heap_min_free`; `berry profile` prints `Default heap`/`Internal heap`. Host tests: cap-aware `test/mocks/mock_system.{h,c}`, `test/test_system.c` adds `test_get_info_distinguishes_default_and_internal_heap` (`test_system` 12/12); `test_shell_berry` 26/26 and `test_shell` 63/63 passed in impacted host run. Hardware verified (2026-11-09): `berry profile`/`mem`/`memory_churn`/`berry gc` OK; labels unambiguous. On this target `system_get_info` default/internal values were numerically identical (cap-query result) while Berry `Heap snapshot` internal metrics (`MALLOC_CAP_INTERNAL`) remained distinct — no longer misleading. Evidence: `docs/reports/2026-11-08-r5-hardware-results-partial.md`.
  - [x] **R5-FU3:** Hardware evidence complete; R5 acceptance criteria marked done and final measurements recorded in `docs/reports/2026-11-08-r5-hardware-results-partial.md` (2026-11-08). Default GC step confirmed 200.

### R6 — Firmware on-demand native module loading
- **Priority:** P1 | **Complexity:** M–L | **ROI:** High | **Risk:** Medium–High
- **Status:** ✅ **Done (2026-11-08, hardware validated including bugfix retest).**
- **Source:** `docs/2026-11-07-ts-feature-extensions.md`, `docs/2026-11-06-memory-pressure-fixes.md`
- **Current state:** `console`, `fs`, `net`, and `ui` are not eagerly loaded at `berry_runtime_init()`. `berry_runtime_init()` installs lightweight lazy proxy globals (tiny dynamic Berry modules with a Berry-closure `member` hook calling internal `cos_lazy_member(module, attr)`). On first bare-global attribute access (e.g. `ui.label(...)`), the proxy loads the real native module from `be_modtab.c` via `be_getmodule()`, replaces the global with the real module, and returns the attribute. `cos_import('@cos/*')` and Berry `import` load real modules on demand and bypass proxies. `json` remains on-demand via R1 (`berry_runtime_ensure_json()`); no lazy global proxy is installed for `json`.
- **Spec / acceptance criteria:**
  - Modules not used by a script/app avoid unnecessary allocation/loading where feasible. ✅ Real native module tables initialize only on first use/import; startup installs only small proxy globals + `cos_lazy_member` helper.
  - Existing apps and SDK usage remain compatible or are migrated with clear compatibility rules. ✅ TS-compiled apps/scripts (`storage/ts/*` → `cos_import`/local bindings) unaffected. Bare-global Berry compatibility preserved via lazy proxies. Rule: prefer `import ... from '@cos/*'` in TS and `import <module>`/`cos_import` in Berry; `json` bare global is not preinstalled (use `import json` or runtime-provided FS JSON/fetch JSON paths).
  - Lazy loading does not introduce reentrancy/VM lifetime bugs. ✅ No issues observed in hardware validation (proxy executes in Berry VM call path; apps/scripts launched and exited cleanly).
  - Memory impact is measured (ties to R2/R5). ✅ R5 checkpoints/profiles captured in validation; real modules load on first use (e.g. breakout/tap_dash exercise `ui` via `cos_import`, scripts exercise `fs`/`net`/`json`/`console` paths). Formal pre/post R6 startup delta can be compared against R2 baselines if needed.
- **Implementation notes:**
  - `components/berry_lang/src/berry_runtime.cpp`: removed eager startup loading for `console/fs/net/ui`; added `native_cos_lazy_member` and closure-based lazy proxy globals; registers `cos_lazy_member` helper. `json` left to R1 path.
  - Bugfix (hardware feedback, 2026-11-08): initial implementation incorrectly left `be_setmember` value on stack and stored member function as global; also used a native `member` hook form that was fragile for `be_module_attr`'s low-level call path. Fixed by popping after `be_setmember` and using Berry closure `member` hooks.
  - Validation: `scripts/validate-berry-scripts.ps1` **24/24** `.be` files compiled. Host tests: `test_shell_berry` **26/26**, `test_berry_source_buffer` **2/2**, `test_shell` **63/63**.
  - Hardware validation (user, 2026-11-08): `tests/r6/lazy_modules.be` passed (`R6 OK`; `import fs`, `cos_import @cos/net`, bare `ui.KEY_ENTER=40`, bare `console.font_size=1`, `import json`). `apps/breakout.be` launched, setup completed, `berry close` GC teardown OK. `apps/tap_dash.be` launched, setup completed, `berry close` GC teardown OK. Earlier breakout `attribute_error` seen during buggy first build did not reproduce after bugfix.
  - Hardware guide: `docs/reports/2026-11-08-r6-hardware-validation-guide.md`.
- **Progress:**
  - [x] Compatibility strategy decided (hybrid lazy proxies + on-demand `cos_import`/`import`; `json` keeps R1 behavior).
  - [x] Runtime/module registration refactor implemented.
  - [x] Host regression/compile validation run.
  - [x] Hardware validation + evidence recorded (bugfix retested).

### R7 — Fetch `include_body` option (P3)
- **Priority:** P1 | **Complexity:** S | **ROI:** Medium | **Risk:** Low
- **Status:** ✅ **Done (2026-11-08, hardware validated).**
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P3
- **Current state:** `net.fetch` accepts optional `include_body: bool` (default `true`). When `false`, the returned `body` is an empty string, avoiding a Berry string copy of the raw HTTP response; JSON parsing and `json`/`json_ok` behavior are unchanged. Invalid `include_body` type is rejected as `ESP_ERR_INVALID_ARG` during option parsing (before queueing the request).
- **Spec / acceptance criteria:** Add optional `include_body: bool` (default `true`). When `false` and JSON parsing succeeds, return parsed JSON but omit/empty raw body to avoid retaining the extra Berry string. Native parsing, `known_fetch_field`, TypeScript SDK, and docs updated. Existing scripts unchanged.
- **Implementation notes:**
  - `components/berry_lang/src/berry_network.cpp`: added `include_body` to `known_fetch_field`; parsed bool `include_body` (default `true`); omitted `body`/`body_len` from `push_fetch_result` when `false` after the existing JSON parse path.
  - `frontend/src/cos-sdk/network.d.ts`: added `FetchRequest.include_body?: boolean` and documented `body` as empty when `include_body` is `false`.
  - Validation: `scripts/validate-berry-scripts.ps1` **25/25** `.be` files compiled (includes `storage/flash_data/tests/r7/include_body.be`). Frontend `npx vitest run` **92 passed / 16 files**.
  - Hardware validation (user, 2026-11-08): `berry exec body.be` (R7 command script) passed offline arg validation; live check `berry exec body.be http://ip-api.com/json` reported good (arg validation passed; JSON fetch/default-body vs `include_body:false` behavior validated per user report). Guide: `docs/reports/2026-11-08-r7-hardware-validation-guide.md`.
- **Progress:**
  - [x] Native implementation.
  - [x] SDK/types/docs updated.
  - [x] Tests added (Berry host syntax validation + hardware test script/guide).
  - [x] Hardware validation completed.

### R8 — HTTP worker stack/TLS measurement & safe tuning (P4/P5)
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Medium–High
- **Status:** ✅ **Done (2026-11-09, evidence-based).** **Stack decision:** keep `HTTP_TASK_STACK=6144` (no reduction). Worst observed HWM **2184/6144** (`used~3960`, **~35.5% margin**, `unit=1`) on HTTPS POST/error paths; clean-boot HTTPS observed 2272/6144. Idle worker shutdown deferred. **Caller-abandon validated:** `delay/5` 300/300ms → `ESP_ERR_TIMEOUT`, immediate follow-up/retry `ESP_ERR_INVALID_STATE` while abandoned worker busy (post-completion retry not captured). **Open follow-ups:** HTTPS PSA/X.509 `-141` under heap pressure (R2/R8 P5), BT-state/`idf.py size` recording, optional post-completion retry capture.
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P4, P5
- **Spec / acceptance criteria:**
  - Add high-water-mark measurement for HTTP worker under GET/POST/HTTPS/max response/timeout/error cases; account for port stack units.
  - Reduce `HTTP_TASK_STACK` only with measured peak + >=25% margin.
  - Profile HTTPS/TLS heap, largest block, and cleanup paths; do **not** disable certificate verification.
  - Optional idle worker shutdown only if lifecycle is race-free and measurably beneficial.
- **Progress:**
  - [x] P4 instrumentation (software): `components/network/src/network_http.c` logs HTTP worker stack HWM after each request (configured stack bytes, `StackType_t` unit, HWM words/bytes, used/peak estimate, method/scheme/status/err). `network_http_get_worker_stack_info()` added; `berry profile` and `berry http-stack`/`berry net-stack` (`components/shell/src/shell_berry_cmd.c`, host tests updated) expose samples. `HTTP_TASK_STACK` remains **6144**.
  - [x] Timeout/abandon response-buffer safety (R2 follow-up, software): on caller timeout, `network_http_fetch` marks request abandoned and returns a stable empty public response snapshot; `http_event`/`perform_request` check abandonment before mutating shared response buffers. Worker may continue until `esp_http_client_perform` completes; next fetch returns `ESP_ERR_INVALID_STATE` while busy. `components/network/include/network.h` docs updated. **True caller-abandon hardware path not reproduced** in captured runs; transport-failure path stable, no crash/stale data observed.
  - [x] Guided hardware validation checklist/report: `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`. Helper script: `storage/flash_data/tests/r8/http_validation.be`.
  - [x] Measurements recorded (hardware, 2026-11-09, Wi‑Fi connected; not clean-boot P5 baseline; BT state/`idf.py size` not recorded): HTTP GET 200 OK. HTTPS POST `postman-echo.com` 200 OK in healthy state (worst HWM 2184/6144); later long-session runs showed PSA/X.509 `-141` on HTTPS POST/GET with extreme heap minima (484→84 observed), while HTTPS `httpbingo.org/bytes/2000` still succeeded in those runs. Max 2000 bytes 200 OK. Oversized 2100 bytes **`ESP_ERR_INVALID_SIZE`**. 3× matrix repeats showed no stack below 2184; repeated HTTP GET ×3 200 OK per run, no progressive heap decrease after GC. Evidence: `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`.
  - [x] Safe tuning applied or explicitly deferred with evidence: **keep `HTTP_TASK_STACK=6144`** (>=25% margin on all observed worst cases). Idle shutdown deferred.
  - [x] Network regression tests passed (hardware, scoped): repeated HTTP/HTTPS 200 OK in healthy states; 2000-byte response OK; oversized (>2048) fails cleanly as `ESP_ERR_INVALID_SIZE`; connect-failure paths no crash; **caller-abandon path validated** (`ESP_ERR_TIMEOUT` + busy `ESP_ERR_INVALID_STATE`). Limitations: PSA `-141` heap follow-up (R2), BT-state/`idf.py size` not recorded, post-abandon-completion retry not captured.

### R9 — Berry command-script resolver + `path` + `ping` pilot
- **Priority:** P1 | **Complexity:** M–L | **ROI:** Medium–High | **Risk:** High
- **Source:** `docs/2026-11-01-improvements.md`
- **Spec / acceptance criteria (summary):** Resolve non-native commands to `.be` scripts (CWD, then configured path dirs); native commands always win; explicit path runs only that script; `main(args)` receives tokenized args and returns int status; scripts run in shared Berry VM without closing active UI app; `path` command list/add/remove/clear with normalized persisted config; `ping.be` pilot preserves existing `ping` contract before native registration is removed. Full spec in source doc.
- **Progress:**
  - [x] Resolver/tokenization design finalized (interactive: alias → native `esp_console_run` → script resolver on `ESP_ERR_NOT_FOUND`; tokenization via `esp_console_split_argv`/existing quoting behavior).
  - [x] `path` config/persistence implemented (`/flash/.cos_path`, normalized absolute dirs, max 8, CWD searched first but not stored).
  - [x] Script execution path implemented (`shell_script_try_exec` / `shell_dispatch_expanded_line`, `berry_runtime_exec_file`, `main(args)`, int status propagation).
  - [~] Host/QEMU tests added. Host unit tests: `test_shell_script` **19/19**, `test_shell` **63/63**, `test_shell_berry` **26/26** (rebuilt/verified 2026-11-08 in `test/build`). QEMU/integration coverage for resolver/`path` not yet added.
  - [x] Hardware validation complete (2026-11-08, user report): R9-HW2 ✅ reboot persistence; R9-HW4 ✅ CWD-first/order/unknown; R9-HW5 ✅ native precedence; R9-HW6 ✅ explicit paths + missing explicit path `script not found` (no fallback); R9-HW8 ✅ `ping.be` pilot contract via explicit path (usage, success `Reply from <address>: time=<rtt> ms`, valid timeout `500`, invalid `99`/`3001`/non-numeric timeout, host failure `ESP_ERR_NOT_FOUND`); R9-HW9 ✅ command script while UI app active OK. R9-HW1/3/7 deterministic cases covered by host unit tests in `test/test_shell_script.c` (**19/19 pass**). Evidence/checklist: `docs/reports/2026-11-08-r9-hardware-validation-guide.md`.
  - [x] Interactive dispatch unified (2026-11-08): `components/shell/src/shell_input.c` now uses `shell_dispatch_expanded_line()` after alias expansion, removing duplicated native→script fallback logic. `shell_dispatch_expanded_line()` contract clarified in `shell_script.h/.c`: returns `ESP_OK` when handled (native/script success or resolver-reported script/path error, `*ret` carries status, default 1 for reported errors) and `ESP_ERR_NOT_FOUND` only when no command/script matched; prevents interactive "Unknown command" on handled explicit-path/script errors. Updated `test/test_shell_script.c` expectations accordingly.
  - [ ] Native `ping` migration decision recorded/executed: native command still registered (bare `ping` remains native). Remove/make unreachable native registration only as a deliberate follow-up after script lookup active + pilot validated; update `README.md` if migrated. Follow-up: audit other `berry_network` map-return stack patterns (only `native_net_ping` restored/correct here). Also fixed in repo during validation: `native_net_ping` return stack, stock scripts `ping`/`list_files`/`fetch_json`/`wget` use bracket access for Berry map results.

### R10 — Frontend TS toolchain hardening
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Low–Medium
- **Status:** ✅ **Done (2026-11-09, Web Worker compilation explicitly deferred by user decision).**
- **Source:** `docs/2026-11-03-ts-sdk.md`
- **Spec / acceptance criteria:**
  - Compilation runs in a dedicated Web Worker (or existing behavior is verified and documented). ✅ Verified: `frontend/src/editor/editor-app.tsx` compiles synchronously via `astToBerry(getAst(...))` on the main thread. Worker isolation intentionally deferred (COS web UI apps are simple; avoid added complexity until compile latency/multi-file needs justify it).
  - Package/layout decision recorded (`frontend/*` integrated vs `packages/cos-sdk` + `packages/cos-typescript`). ✅ Keep integrated `frontend/src/cos-sdk` + `frontend/src/ide/*` for now; extract to packages when reuse/publication needs arise.
  - Monaco language features and SDK `addExtraLib()` registration are minimal/correct. ✅ Audited. Added `frontend/src/ide/cos-sdk-manifest.ts` + manifest test; registration applies all global/module libs and `@cos/*` path mappings (including `fs/filesystem`, `net/network` aliases). Monaco language imports are minimal for supported editor file types.
  - Device filesystem read/write/deploy flows handle auth and errors predictably. ✅ Audited existing flows (`cos-files`, `device-access`, editor save/deploy, file tree). Extended `frontend/src/ide/files/cos-files.test.ts` for predictable error/auth/path cases.
- **Report:** `docs/reports/2026-11-08-r10-frontend-toolchain-hardening.md`
- **Progress:**
  - [x] Worker isolation verified and explicitly deferred (documented).
  - [x] Layout decision documented.
  - [x] SDK registration refactored for testability; manifest drift test added.
  - [x] Device API error/auth tests extended; targeted frontend tests and `tsc --noEmit` passed.
  - [ ] Optional follow-up: move compilation to a Vite Web Worker if latency/multi-file compilation becomes a concern.

### R11 — SDK single-source-of-truth / binding audit
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Low–Medium
- **Status:** ✅ **Done (2026-11-09, pragmatic audit/validation).** No hardware validation required.
- **Source:** `docs/2026-11-03-ts-sdk.md`, `docs/2026-11-07-ts-feature-extensions.md`
- **Spec / acceptance criteria:** SDK declarations, module exports, and docs match actual Berry/COS bindings. Prefer generation or automated validation to prevent drift. Cover `ui`, `console`/`log`, `fs`, `net`, `action`, Berry built-ins, and `@cos/*` module semantics.
- **Report:** `docs/reports/2026-11-08-r11-sdk-binding-audit.md`
- **Progress:**
  - [x] API inventory created. Explicit COS native binding/global/module-specifier inventory in `frontend/src/ide/cos-binding-inventory.ts`, derived from `components/berry_lang/src/berry_ui.cpp`, `berry_network.cpp`, and `berry_runtime.cpp`. `ui.init` and runtime helpers (`cos_import`/`cos_module`/`cos_lazy_member`) treated as internal/non-script-surface where appropriate.
  - [x] Drift fixes/generation approach implemented (pragmatic). `frontend/src/ide/cos-sdk-manifest.ts` now registers `@berry/*` specifiers alongside `@cos/*` (runtime/compiler already supported them; Monaco paths did not). `frontend/src/cos-sdk/json.d.ts` documents R1/R6 lazy global/module availability. Full declaration generation deferred; inventory + automated audit chosen for maintainability now.
  - [x] Validation added. `frontend/src/ide/cos-binding-audit.ts` + `cos-binding-audit.test.ts` cross-check C++ `be_native_module_*`/`be_regfunc` registrations against inventory and validate SDK global declarations/module wrappers/manifest specifiers. Updated `cos-sdk-manifest.test.ts`. Validation (2026-11-09): `npx vitest run src/ide/cos-binding-audit.test.ts src/ide/cos-sdk-manifest.test.ts` **10 passed / 2 files**; `npx tsc --noEmit` passed.

### R12 — Milestone 4 hardware validation
- **Priority:** P1 | **Complexity:** S (effort) / M (logistics) | **ROI:** Medium | **Risk:** Low
- **Source:** `docs/2026-10-improvements.md` Milestone 4
- **Spec / acceptance criteria:** Validate touch scrollback gestures and connected-network `ping`/`nslookup` commands on supported CYD hardware; record results in the source milestone doc and here.
- **Progress:**
  - [ ] Hardware test run completed.
  - [ ] Results recorded.

### R13 — Memory-conscious compiler rules + resource lifecycle contract
- **Priority:** P2 | **Complexity:** M–L | **ROI:** Medium | **Risk:** Medium
- **Source:** `docs/2026-11-04-after-ts-optimizations.md`
- **Spec / acceptance criteria:** Configurable, documented diagnostics/rules for predictable app structure and cleanup (globals, lifecycle, hot callbacks, unsupported memory-sensitive patterns). Standardize `CosApplication`/resource cleanup patterns; native API support only where needed and measured.
- **Progress:**
  - [ ] Rule set defined.
  - [ ] Diagnostics/SDK contract implemented.
  - [ ] Docs/tests added.

### R14 — Compiler profiles, debug metadata, source maps, editor diagnostics
- **Priority:** P2 | **Complexity:** M–L | **ROI:** Medium | **Risk:** Medium
- **Source:** `docs/2026-11-04-after-ts-optimizations.md`
- **Spec / acceptance criteria:** Dev/prod compiler profiles; production options evaluated (including `BE_DEBUG_RUNTIME_INFO` implications); source-map strategy for mapping Berry/runtime errors to TS; editor memory diagnostics surfaced from runtime metrics.
- **Progress:**
  - [ ] Profile design.
  - [ ] Implementation/experiments.
  - [ ] Diagnostics UX added.

### R15–R17 — Bytecode, UI allocation policy, solidification (experimental)
- **Priority:** P2 | **Complexity:** L | **ROI:** Low–Medium/targeted | **Risk:** Medium–High
- **Source:** `docs/2026-11-04-after-ts-optimizations.md`
- **Guardrails:** Each is a separate, benchmark-gated experiment. `.bec` must remain optional and version-tracked; UI allocation changes must compare retention vs fragmentation; solidification applies to firmware-bundled code, not general dynamic TS app deployment.
- **Progress:** track individually when started.

### R18 — `async`/`await` runtime design + implementation
- **Priority:** P2 | **Complexity:** XL | **ROI:** Medium–High | **Risk:** High
- **Source:** `docs/2026-11-07-ts-feature-extensions.md`
- **Spec / acceptance criteria:** Design first: concurrency model on constrained hardware, interaction with Berry VM single-threading/tasking, cancellation, timers, networking callbacks, and memory/resource lifetime. Only then add compiler syntax lowering; no silent desugaring.
- **Progress:**
  - [ ] Runtime/concurrency design doc approved.
  - [ ] Minimal implementation.
  - [ ] Compiler support + tests.

### R19 — In-browser COS simulator
- **Priority:** P3 | **Complexity:** XL | **ROI:** High (long-term DX) | **Risk:** High
- **Source:** `docs/2026-11-05-in-browser-cos-simulator.md`
- **Spec / acceptance criteria (staged):** Pin Berry revision/config parity with firmware; build reproducible Berry WASM; run VM in dedicated Worker with watchdog/termination/recovery; define COS platform abstraction seam (display/input/clock/timers/filesystem/commands/logging); implement `Renderer`-backed canvas; add explicit capability labels for simulated/stubbed hardware features. Full spec in source doc.
- **Progress:**
  - [ ] Berry parity/toolchain spike.
  - [ ] Worker VM lifecycle.
  - [ ] Platform abstraction + renderer.
  - [ ] Input/timers/filesystem/commands.
  - [ ] IDE integration.

### R20 — 2026-10 deferred backlog
- **Priority:** P3 | **Complexity:** M–XL | **ROI:** Mixed | **Risk:** Medium–High
- **Source:** `docs/2026-10-improvements.md` deferred follow-ups
- **Items:**
  - [ ] Shell pipes/redirection (`>` / `>>`) — requires parsing/streaming/file-failure semantics design.
  - [ ] Shell line-editor refactor seam for testability and future input owners.
  - [ ] Larger apps: file manager, text editor, calendar, broader UI showcase (separately scoped).
  - [ ] Image preview / Berry image support — requires format, memory, decoder feasibility assessment.

### R21 — Unify `berry run`/`berry exec` on `main(args)` contract
- **Priority:** P2 | **Complexity:** L | **ROI:** Medium–High (DX/consistency) | **Risk:** High (breaking migration)
- **Source:** follow-up to R9 command-script model; design discussion 2026-11-08
- **Problem:** `berry run` and `berry exec` expose different execution/lifecycle contracts (app/script execution with active-app reset vs command-script `main(args)` without UI disturbance), causing UX confusion and duplicated mental models.
- **Decision (proposed):** Compatibility is not the top priority. Unify on a single required `main(args)` entrypoint for runnable `.be` files. Default execution is command/non-intrusive; UI apps are launched explicitly from `main(args)` via a dedicated runtime API (exact API TBD). `berry exec` may remain a temporary alias to unified `berry run`.
- **Spec / acceptance criteria:**
  - `berry run <path> [args...]` is the unified command; args are passed as a list excluding the script path; `main(args)` returns an integer shell status.
  - Missing `main`, non-integer/out-of-range return values, and runtime errors produce clear nonzero failures.
  - The default path does not close/disturb an active UI app or alter UI/console app state.
  - The explicit app-launch API performs documented app lifecycle (replacing/closing active app as specified, console/scrollback/UI setup, `setup`/`teardown` integration) and has defined status semantics.
  - R9-resolved command scripts use the same execution contract/path.
  - All in-repo `.be` entrypoints (`storage/flash_data`, tests/scripts/apps/autoexec) are migrated; TS compiler/templates/SDK docs emit or document the unified shape.
  - Help/README/completion/tests/hardware validation are updated.
- **Dependencies:** R9 stabilized/decided; coordinate with R10/R11 (and R14 if active). Prefer reusing R9's `main(args)` command-script foundation rather than widening R9 mid-validation.
- **Progress:**
  - [ ] Contract/spec approved (app-launch API, lifecycle/status semantics, alias/deprecation).
  - [ ] Runtime + shell implementation.
  - [ ] Repo `.be` entrypoints migrated.
  - [ ] TS toolchain/SDK/templates updated.
  - [ ] Tests/docs/hardware validation complete.

## Progress log

| Date | Update | Evidence / notes |
|---|---|---|
| 2026-11-08 | Consolidated plan created. Source docs reviewed and status annotations added in `2026-11-01`–`2026-11-07`. | Findings reflect repo state at commit `3659f62` (latest shown during review) plus earlier `e0bf05b`/`8d32f84`. |
| 2026-11-08 | R1 started: removed eager `json` global install from `berry_runtime_init()`. | `components/berry_lang/src/berry_runtime.cpp`; `json` remains in `be_modtab.c`; `berry_runtime_ensure_json()` still used by `fs.read_json/write_json` and `net.fetch(..., response_type="json")`. Firmware rebuild + hardware validation pending (user). |
| 2026-11-08 | R1 hardware validation (partial): boot/non-JSON OK; `fs.read_json`/`write_json` OK; `net.fetch` JSON path exercised on connect failure (no crash, correct error shape). Successful JSON HTTP parse still recommended for full acceptance. | User logs: `mem`/`hello.be` OK; `/sdcard/tests/validate-json.be` → `write_json` true, `read_json` returned `{value:{n:42,s:"r1",ok:true},ok:true}` and fields correct; `/sdcard/tests/fetch-invalid.be` → `ok=false,status=0,json_ok=false,json=nil,error=ESP_ERR_HTTP_CONNECT`. Attempted `https://raw.githubusercontent.com/.../frontend/tsconfig.json` → same `ESP_ERR_HTTP_CONNECT` (likely device network/TLS/connectivity), so JSON parse-on-success not yet observed. |
| 2026-11-08 | R1 complete. | Successful `net.fetch(..., response_type="json")` after connectivity fix: `true, 200, true, {'slideshow': ...}` (parsed JSON returned). Acceptance criteria satisfied: no eager JSON load at startup; FS JSON works; fetch JSON works; repeated JSON ops supported by cached/lazy ensure path; VM recreate implies fresh globals; non-JSON scripts/apps unchanged. |
| 2026-11-08 | R2 in progress: profiling implementation/checklist. | Added `berry profile` (system heap + Berry GC metrics) and `berry gc` shell commands, `berry_runtime_gc_collect()` API + host mock, P7 checklist/report template in `docs/reports/2026-11-08-r2-profiling-checklist.md`, and deployable Berry regression scripts in `storage/flash_data/tests/r2/`. Hardware/IDF measurements and regression execution remain pending (user validation). |
| 2026-11-08 | R2 hardware analysis (partial). | User provided `idf.py size`/components and clean-boot reruns A–D in `docs/reports/2026-11-08-r2-profiling-checklist.md`. FS text/JSON pass. HTTP success is functional but costly (`min=3288`, `largest=6912` after 158 B). HTTPS fails from clean boot (`mbedtls_ssl_setup -0x008D` / connect alloc failure). Timeout/error paths can collapse native heap (`min 24–76`, `largest 384–1920`, BT malloc failures, `ESP_ERR_NO_MEM`), with no Berry GC recovery. Report updated with anomalies and prioritized improvements (R8/P4 stack HWM, R8/P5 mbedTLS peak, timeout/cleanup audit). |
| 2026-11-08 | R2 completed as tracker item; remaining work moved to follow-ups. | Deliverables committed (`docs/reports/2026-11-08-r2-profiling-checklist.md`, `berry profile`/`berry gc`, `storage/flash_data/tests/r2/*`). Hardware runs showed HTTPS progressed (`-0x008D` TLS alloc → `-141` PSA insufficient memory during cert verification) after memory tuning (`sdkconfig.defaults` mbedTLS 4KB/variable buffers, `HTTP_TASK_STACK` 6144, `NETWORK_HTTP_BODY_MAX` 2048/headers reduced, smaller esp_http_client buffers) — committed in `6a48ecf` plus subsequent uncommitted tuning. Follow-ups: HTTPS PSA/RAM with verification, timeout/abandon cleanup, BT coexistence, and remaining P7 regression/clean-boot checkpoint runs (see R2 follow-ups, R8). |
| 2026-11-08 | R3 implementation complete; hardware validation pending. | TS→Berry compiler fixes in `frontend/src/ide/berry/*` (`.length` → `size(expr)`, numeric literal source preservation, int-literal division / unary + / typeof/void/delete diagnostics). SDK numeric harmonization in `frontend/src/cos-sdk/language.d.ts` and `modules/math.d.ts`. Regenerated `storage/flash_data/*.be` outputs. Added `docs/2026-11-08-ts-berry-semantic-conformance.md` and `docs/reports/2026-11-08-r3-hardware-checklist.md`. Frontend tests: `npx vitest run` → 82 passed, 16 files. |
| 2026-11-08 | R3 completed. | User validated what was possible; R3 marked complete. Follow-up: IDE/Monaco SDK registration updated for `@cos/*` module paths (`frontend/src/ide/register-cos-sdk.ts`) to resolve imports such as `@cos/math`. |
| 2026-11-08 | R4 complete: `try`/`catch`/`throw` compiler lowering. | `frontend/src/ide/berry/statement-generator.ts` lowers `throw`→`raise` and `try/catch`→Berry `try/except .. [as e]/end`; `finally` and destructuring catch bindings diagnosed. Berry exception syntax confirmed from pinned tests (`build/berry-source/tests/exceptions.be`, `class.be`, `compiler.be`, `division_by_zero.be`). R4 unit tests added in `frontend/src/ide/ast-to-berry.test.ts` (37 passed). User hardware validation: `berry run /sdcard/tests/trycatch.be` produced expected output ending with `R4 OK`. Checklist: `docs/reports/2026-11-08-r4-hardware-checklist.md`. |
| 2026-11-08 | R5 implementation complete; hardware validation/follow-ups pending. | All R5 code/docs/test assets delivered: `[R5]` lifecycle checkpoints (1–7, single consolidated GC-complete log), extended `berry_runtime_memory_info_t`/`berry profile`/GC-step API (`default 200`), source-buffer release verified and load path hardened (`vfs_stat`+read, post-stat internal/8bit fallback for short-lived buffer), `berry_ui_end_app` VM guard, `berry gc-step` shell command, `tests/r5/memory_churn.be`, `scripts/validate-berry-scripts.ps1` (22/22 `.be`), `scripts/berry_conf.host_validation.h`, hardware guide/results. Host tests: `test_shell_berry` 26/26, `test_berry_source_buffer` 2/2, `test_shell` 63/63. Hardware (partial): churn stable; Breakout lifecycle checkpoints 1–7 seen on successful run; source-buffer `ESP_ERR_NO_MEM` diagnosed/fixed (reflash pending); **critical follow-up**: post-close low-heap `LoadStoreError` stdio corruption panic during logging (`stdout_write_hook`/`fwrite`, EXCVADDR `0x40000000`). Follow-ups: R5-HW1/2/3, R5-FU1 (critical), R5-FU2, R5-FU3. |
| 2026-11-08 | R5-HW2 complete (Tap Dash 10 cycles). | User hardware log: 10 `tap_dash.be` open/close cycles. Post-warm-up steady state `berry_gc=4342`, `reclaimed=5320`, `heap_free=19056→29056` (cycle 8 variance explained by BT HID auto-connect failure settling), `heap_largest=9216`, watermarks stable (`heap_min=2920`, `internal_min=26084`, `internal_largest=22528`). No progressive leak. With Breakout 7-cycle evidence, restart-loop regression criterion (two apps) satisfied. Updated `docs/reports/2026-11-08-r5-hardware-results-partial.md`, `docs/2026-11-08-consolidated-improvement-plan.md`. |
| 2026-11-08 | **R5 complete (accepted). R5-HW3 GC benchmarks done; default GC step 200 confirmed.** | R5-HW3 identical workload (Breakout ×5 + `memory_churn.be` ×3; on-device churn file loaded as 0 bytes, Breakout comparison valid): rates **200/150/125** produced identical steady/post-close metrics (`managed=4551`, `reclaimed=10555–10581`, `heap_free=28400`, `heap_largest=8704`, `heap_min=2920`, `internal_min=26084`, `internal_largest=22528`), **no responsiveness impact noticed**. `berry gc-step 200` restored/confirmed (`step_rate=200`). R5 acceptance criteria satisfied (checkpoints 1–7, source-buffer lifetime/release+fallback, restart-loop regression, GC benchmark evidence, host `.be` validation/tests). Open follow-ups: **R5-FU1 (critical stdio/heap corruption root cause)**, R5-FU2 heap reporting display, optional churn redeploy/worst-case reload recheck. Evidence: `docs/reports/2026-11-08-r5-hardware-results-partial.md`. |
| 2026-11-08 | **R6 implemented (firmware): on-demand native module loading with lazy compatibility proxies. Hardware validation pending.** | `components/berry_lang/src/berry_runtime.cpp`: removed eager startup loading of `console/fs/net/ui` globals; `berry_runtime_init()` installs lightweight lazy proxy globals (dynamic module + Berry closure `member` hook). First bare-global attribute access loads the real native module via `be_getmodule()`, replaces the global, and returns the attribute; real module tables are not initialized at startup. `cos_import('@cos/*')` and Berry `import` remain on-demand. `json` unchanged from R1. Compatibility: TS-compiled shipped apps/scripts already use `cos_import`; bare `ui.*`/`net.*`/`fs.*`/`console.*` still works via proxy. Validation: `scripts/validate-berry-scripts.ps1` **24/24** `.be` files compiled (after R6 test script added); host tests `test_shell_berry` **26/26**, `test_berry_source_buffer` **2/2**, `test_shell` **63/63**. Hardware guide: `docs/reports/2026-11-08-r6-hardware-validation-guide.md`. |
| 2026-11-08 | **R6 bugfix validated on hardware; R6 accepted complete.** | User report: `tests/r6/lazy_modules.be` failed at bare `ui.KEY_ENTER` (`'function' value has no attribute 'KEY_ENTER'`). Root causes: (1) `be_setmember` does not pop, so `be_setglobal` stored the member function instead of the proxy module (`console/fs/net/ui` globals became functions). (2) Native `member` hook form was fragile for `be_module_attr`'s low-level call/return path. Fix in `components/berry_lang/src/berry_runtime.cpp`: `be_setmember` value popped; proxy `member` uses a Berry closure (`def(k) return cos_lazy_member('<name>', k) end`) calling `cos_lazy_member` native helper to load/replace real module and return attribute (with attribute errors). Hardware retest (user): `tests/r6/lazy_modules.be` → `R6 OK` (`R6 bare ui.KEY_ENTER 40`, `R6 bare console.font_size 1`, `import fs`/`cos_import @cos/net`/`import json` all OK). `apps/breakout.be` launched/setup/closed cleanly; `apps/tap_dash.be` launched/setup/closed cleanly. Earlier breakout `attribute_error` from buggy first build/session did not reproduce. R6 acceptance criteria satisfied (on-demand load, compatibility preserved, no observed VM lifetime issues, R5 checkpoint/profile evidence captured). |
| 2026-11-08 | **R7 implemented (firmware/SDK/docs): fetch `include_body` option (P3). Hardware validation pending.** | `components/berry_lang/src/berry_network.cpp`: added `include_body` to `known_fetch_field`, parsed bool `include_body` (default `true`, invalid type → `ESP_ERR_INVALID_ARG` before fetch), and returns empty `body` when `include_body=false` while preserving JSON parse/`json_ok`/`json` behavior. `frontend/src/cos-sdk/network.d.ts`: added `FetchRequest.include_body?: boolean` and documented empty `body` cases. `docs/2026-11-06-memory-pressure-fixes.md` P3 marked implemented (hardware validation pending). Added `storage/flash_data/tests/r7/include_body.be` and `docs/reports/2026-11-08-r7-hardware-validation-guide.md`. Validation: `scripts/validate-berry-scripts.ps1` **25/25** `.be` files compiled; frontend `npx vitest run` **92 passed / 16 files**. ESP-IDF firmware build and live network/heap validation not run in this environment (hardware required). |
| 2026-11-08 | **R7 complete (hardware validated).** | User hardware validation: `berry exec body.be` (R7 command script; `_argv` fixed to `def main(args)`, use `berry exec`) passed offline arg validation (`invalid include_body type rejected`, `bool values accepted`, `R7 ARG OK`). Live check `berry exec body.be http://ip-api.com/json` reported good (JSON fetch/default-body vs `include_body:false` behavior validated per user report). R7/P3 acceptance criteria satisfied; plan/docs updated. |
| 2026-11-08 | **R9 hardware validation partial.** | User report: created `/sdcard/scripts`, added to `path`, placed `be_ping.be` with custom message (confirmed Berry script executed, not native), ran successfully as `be_ping.be` and `be_ping`; path list persisted across restart; remove/clear succeeded. R9 moved to in-progress (impl/host tests done). Pending: R9-HW3–HW9 full coverage (`ping` pilot explicit-path/network contract, native precedence, explicit paths, quoting/status/errors, UI app isolation) and native `ping` migration decision. Guide: `docs/reports/2026-11-08-r9-hardware-validation-guide.md`. |
| 2026-11-08 | **R9 hardware validation continued; `net.ping` Berry binding bug fixed.** | Results: R9-HW5 ✅ native `ping` precedence. R9-HW6/R9-HW4 partial: `path clear` + `be_ping` from `/sdcard/scripts` CWD; from `/flash` relative explicit `../sdcard/scripts/be_ping` and `../sdcard/scripts/be_ping.be` ran. R9-HW3 partial: `path add /sdcard/asdasd` correctly rejected. R9-HW9 partial: breakout `berry run`/close full+relative path OK; script-while-UI-app-active not tested. R9-HW8 blocked: `be_ping.be 127.0.0.1 1000` → `map has no attribute 'ok'` (`ESP_FAIL`). Root cause/fix (workspace): `native_net_ping` in `components/berry_lang/src/berry_network.cpp` removed erroneous `be_pop(vm,1)` before `be_return(vm)` (Berry `be_returnvalue` reads `top-1`). Reflash required; retest ping pilot + remaining HW items. Follow-up: audit other `berry_network` map-return stack paths. |
| 2026-11-08 | **R9 HW continued after reflash; ping script map-access fix.** | User: rebuilt/reflashed (has `native_net_ping` fix). `ping 192.168.0.1` (native) OK; `be_ping` no-args usage OK; `be_ping 192.168.0.1` still failed `'map' value has no attribute 'ok'`. `../sdcard/scripts/be_ping.be` usage OK from `/flash` (HW6). Ping worked while running breakout (HW9 signal). Quotes still work (HW7 partial). Cause: scripts used dot access on plain Berry maps (`result.ok`) — maps require bracket access (`result["ok"]`). Fixed in repo: `storage/flash_data/scripts/ping.be`, `storage/ts/scripts/ping.ts`, and `list_files`/`fetch_json`/`wget` (.be+.ts). User must update SD `be_ping.be` and/or redeploy `/flash/scripts/ping.be`, then retest HW8. HW6 still needs missing-explicit-path/no-fallback (`not Unknown command`); HW9 needs command-script-while-UI-app-active. |
| 2026-11-08 | **R9 HW8 diagnosis corrected; `native_net_ping` restored.** | Pingprobe (after script updated to bracket access): `print(r)=(unknown value)`, `type(r)=map`, `r["ok"]` → "value 'map' does not support subscriptable". Diagnosis: original `native_net_ping` was correct (`be_newobject` → `[instance,.p]`, populate `.p`, `be_pop(1)` remove `.p`, return instance); my interim "fix" removed `be_pop` and returned raw `.p` (broken). Reverted/restored correct `be_pop` in `components/berry_lang/src/berry_network.cpp`. User needs rebuild+reflash with restored C++ and bracket-access script, then retest HW8. |
| 2026-11-08 | **R9 interactive dispatch unified (small follow-up).** | `components/shell/src/shell_input.c` now dispatches expanded lines via `shell_dispatch_expanded_line()` (removes duplicated native-to-script fallback/handled mapping). Clarified `shell_dispatch_expanded_line()` contract in `components/shell/src/shell_script.h/.c`: returns `ESP_OK` when handled (native command or script ran/reported; `*ret` carries status, 1 for resolver-reported errors) and `ESP_ERR_NOT_FOUND` only when nothing matched, avoiding ambiguous handled+error codes and preventing "Unknown command" on explicit-path/script-reported errors. Updated `test/test_shell_script.c` expectations for handled explicit-missing/directory/runtime-error cases. Host tests rebuilt/passed in `test/build`: `test_shell_script` 19/19, `test_shell` 63/63, `test_shell_berry` 26/26. No firmware/hardware build run here; interactive behavior unchanged in intent but should be sanity-checked on device if convenient (explicit missing script should still print only resolver message; unknown bare command still prints `Unknown command`). |
| 2026-11-09 | **R5-FU2 resolved (heap reporting display): code, host tests, and hardware verified.** | `components/system/src/system.c`: `system_get_info()` reports default heap via `MALLOC_CAP_DEFAULT` and internal via `MALLOC_CAP_INTERNAL\|MALLOC_CAP_8BIT`; `system.h` comments clarified. `components/berry_lang/src/berry_runtime.cpp`: `berry_runtime_get_memory_info()` uses `MALLOC_CAP_DEFAULT` for `heap_free/heap_min_free`. `components/shell/src/shell_berry_cmd.c`: `berry profile` prints `Default heap`/`Internal heap`. Host tests: cap-aware `test/mocks/mock_system.{h,c}` + `test/test_system.c` `test_get_info_distinguishes_default_and_internal_heap`; impacted run passed (`test_system` 12/12, `test_shell_berry` 26/26, `test_shell` 63/63). **Hardware verified (user, 2026-11-09):** `berry profile`/`mem` labels unambiguous; `memory_churn` + `berry profile` + `berry gc` behaved normally (managed 7288→2695, reclaimed 4593). Note: on this target `system_get_info` default/internal lines can be numerically identical (cap-query result) while Berry `Heap snapshot` internal metrics (`MALLOC_CAP_INTERNAL`) remain distinct; reporting no longer misleading. Evidence: `docs/reports/2026-11-08-r5-hardware-results-partial.md`. |
| 2026-11-09 | **R10 complete (Web Worker compilation explicitly deferred).** | Frontend-only R10 hardening: verified compilation runs on main thread (`frontend/src/editor/editor-app.tsx` → `astToBerry(getAst(...))`) and recorded user-confirmed deferral of Worker isolation (simple COS web UI apps; avoid complexity). Recorded integrated package/layout decision (`frontend/src/cos-sdk` + `frontend/src/ide/*`). Added `frontend/src/ide/cos-sdk-manifest.ts` and `cos-sdk-manifest.test.ts` for Monaco SDK/addExtraLib coverage and `@cos/*` alias drift checks. Extended `frontend/src/ide/files/cos-files.test.ts` for predictable device API auth/error/path behavior. Updated `docs/2026-11-03-ts-sdk.md` status and added `docs/reports/2026-11-08-r10-frontend-toolchain-hardening.md`. Validation: `cd frontend && npx vitest run src/ide/files/cos-files.test.ts src/ide/cos-sdk-manifest.test.ts src/ide/cos-sdk.test.ts` (**18 passed / 3 files**), `npx tsc --noEmit` passed. No hardware validation required. |
| 2026-11-08 | **R21 added to plan: unify `berry run`/`exec` on `main(args)`.** | Added R21 (P2, M9, not started) as a follow-up to R9. Proposed contract (compatibility not top priority): require `main(args)` for all runnable `.be` entrypoints; unified `berry run <path> [args...]` with integer status; default command/non-intrusive execution; explicit runtime API for UI app launch/lifecycle; `berry exec` may remain a temporary alias. Dependencies: R9 stabilized/decided; coordinate with R10/R11/R14 as relevant. No code changes made. |
| 2026-11-09 | **R11 complete (pragmatic SDK binding audit/validation).** | Added binding inventory `frontend/src/ide/cos-binding-inventory.ts` (COS `ui`/`console`/`fs`/`net`, public globals `log`/`action`, Berry built-in module specifiers) and pure audit helpers `frontend/src/ide/cos-binding-audit.ts`. New `cos-binding-audit.test.ts` cross-checks C++ `be_native_module_*`/`be_regfunc` registrations and validates SDK global declarations/module wrappers/manifest specifiers (`ui.init` treated as internal). Drift fix: `cos-sdk-manifest.ts` registers `@berry/*` aliases alongside `@cos/*` (runtime/compiler already supported). `json.d.ts` clarifies R1/R6 lazy global/module availability. Updated `cos-sdk-manifest.test.ts`, `docs/2026-11-03-ts-sdk.md`, and added `docs/reports/2026-11-08-r11-sdk-binding-audit.md`. Validation: `cd frontend && npx vitest run src/ide/cos-binding-audit.test.ts src/ide/cos-sdk-manifest.test.ts` (**10 passed / 2 files**); `npx tsc --noEmit` passed. No hardware validation required. |
| 2026-11-09 | **R8 progress (software): timeout/abandon safety + measurement guide + `berry http-stack`.** | Implemented R8/R2 timeout-safety hardening in `components/network/src/network_http.c`: timed-out `network_http_fetch` marks request abandoned and returns a stable empty public response snapshot; `http_event`/`perform_request` check abandonment before mutating shared response buffers (request-id guarded, mutex-checked). `components/network/include/network.h` documents timeout response semantics. Added focused `berry http-stack` / `berry net-stack` command (`components/shell/src/shell_berry_cmd.c`) for R8 manual matrix capture; host tests `test_shell_berry` **29/29** pass. Added guided hardware checklist `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md` (P4 HWM matrix, P5 HTTPS/TLS heap with cert verification, timeout/busy/retry checks, stack tuning rule >=25% margin) and all-in-one helper `storage/flash_data/tests/r8/http_validation.be` (Berry syntax validated 26/26 via `scripts/validate-berry-scripts.ps1`; automatable subset only). `HTTP_TASK_STACK` unchanged (6144) pending hardware evidence. **Not complete:** hardware measurements, safe tuning decision, and network regression validation remain pending. |
| 2026-11-09 | **R8 hardware evidence (partial, Wi‑Fi connected).** | User ran `http_validation.be` after Wi‑Fi connect. HTTP GET `http://example.com/` 200 OK (`body_len=577`), stack HWM `4488/6144` (`used~1656`). HTTPS GET `https://example.com/` 200 OK with cert verification (no PSA/X.509 `-141` in this run), stack HWM `2280/6144` (`used~3864`, ~37% single-sample margin); heap watermark dipped to 2500/internal 25664 around HTTPS peak then recovered after GC, largest block 22528 stable. Repeated HTTP GET ×3 all 200, no progressive heap decrease after GC. Timeout case to `10.255.255.1` returned `ESP_ERR_HTTP_CONNECT` (transport connect failure) — **not** a true caller-timeout/worker-abandon case; immediate follow-up succeeded. Gaps remain: clean-boot P5/BT state, POST/HTTPS-POST/max/oversized-body, 3× worst-case repeats, true abandon path validation, `idf.py size`. Evidence appended to `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`; plan R8 marked partial. |
| 2026-11-09 | **R8 hardware evidence extended (POST + max body, partial).** | Second run: `http_validation.be http://example.com/ https://example.com/ https://postman-echo.com/post https://httpbingo.org/bytes/2000`. HTTPS POST 200 OK (`body_len=322`), **worst stack HWM `2184/6144` (`used~3960`, ~35.5%)**; heap watermark dropped to **1272/internal min 24436** around HTTPS POST (severe peak, no OOM/crash, recovered after GC, largest 22528 stable). HTTPS GET 200 OK. Max HTTPS response 200 OK (`body_len=2000`). Repeated HTTP ×3 200 OK, stable after GC. Timeout still `ESP_ERR_HTTP_CONNECT` (not true caller abandon). Remaining: oversized (>2048) check, 3× worst-case repeats, clean-boot/BT-state P5, true abandon-path evidence, `idf.py size`. Docs updated: `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`. |
| 2026-11-09 | **R8 closed (evidence-based).** | User completed oversized + 3× matrix runs. Oversized `https://httpbingo.org/bytes/2100` failed cleanly as **`ESP_ERR_INVALID_SIZE`** (HWM 2184/6144 on error path). Worst observed stack remained **2184/6144 (~35.5%)** across all captured cases — **decision: keep `HTTP_TASK_STACK=6144`**, no reduction. In long-session repeats, PSA/X.509 cert verification failed with **`-141`** on some HTTPS endpoints under extreme heap pressure (watermarks down to 84 bytes observed), but no crash; `bytes/2000` HTTPS and repeated HTTP 200 cases still succeeded, and GC recovered heap. R8 software/instrumentation/safety/measurement objectives satisfied with evidence; **open follow-ups:** R2 HTTPS/PSA `-141` heap/BT coexistence, BT-state recording, `idf.py size`, optional post-abandon-completion retry capture. Evidence: `docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`. |
| 2026-11-09 | **R8 final evidence: caller abandon + clean-boot P5.** | Manual delay test (`httpbingo.org/delay/5`, 300/300ms): `ESP_ERR_TIMEOUT`, immediate follow-up `ESP_ERR_INVALID_STATE`, immediate retry also `ESP_ERR_INVALID_STATE` while abandoned worker still busy (~5s) — confirms R8 abandon safety/busy serialization (later post-completion retry not captured in snippet). Clean-boot-adjacent run (Wi‑Fi `AP+STA`, `mordor` -69dBm, BT state not recorded): HTTPS GET `example.com` 200 OK (HWM 2272/6144), HTTPS POST `postman-echo.com` failed PSA `-141` (min fell to 1968), subsequent HTTPS GET/bytes-2000/repeated HTTPS GETs succeeded; no crash. Docs updated with final evidence. |

## Validation checklist (use when closing items)

- [ ] Relevant unit tests added/updated and passing.
- [ ] Integration/QEMU tests added/updated where applicable.
- [ ] Firmware build passes for affected targets.
- [ ] Formatting/lint/static analysis checks pass.
- [ ] Memory/runtime changes include before/after measurements (heap, largest block, watermark, `.bss`/binary size as relevant).
- [ ] Docs/source specs updated to match shipped behavior.
- [ ] Hardware validation recorded when behavior depends on physical CYD/touch/network.
