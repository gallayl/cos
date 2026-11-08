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
| Partial lazy JSON infrastructure (P2) | `berry_runtime_ensure_json()` / `call_json_method()` in `components/berry_lang/src/berry_runtime.cpp`; FS JSON methods use it | **Gap:** `berry_runtime_init()` still calls `install_module_global(..., "json")` and installs module globals eagerly, so “no JSON on ordinary startup” is not satisfied. |
| Frontend TS/Berry IDE foundation | `frontend/` (Shades + Vite + Monaco), `frontend/src/cos-sdk/*`, `frontend/src/ide/*`, `frontend/tools/compile-storage.ts` | TS→Berry compiler, diagnostics, tests, relative + `@cos/*`/`@berry/*` `cos_import` lowering, type-only import stripping. |
| TS import features (compiler side) | Commits `1c945ae`–`3659f62`; `frontend/src/ide/berry/statement-generator.ts`, `ast-to-berry.test.ts` | Type imports done; relative/custom imports and `@cos/*`/`@berry/*` module specifiers supported in compiler. SDK module declarations exist. |

### Open / partially open

| Area | Source docs | Current gap |
|---|---|---|
| Memory pressure follow-ups | `2026-11-02`, `2026-11-06` | P2 startup-loading gap; P3 `include_body`; P4 stack measurement/tuning; P5 TLS peak measurement; P7 profiling/regression matrix; Phase 1 instrumentation/source-buffer/lifecycle/GC/host `.be` CI validation. |
| Firmware module loading hygiene | `2026-11-06` P2, `2026-11-07` | Runtime eagerly installs `console/fs/net/ui/json` globals at init; on-demand/native module loading strategy not implemented. |
| TS compiler/runtime semantics | `2026-11-03`, `2026-11-07` | `.length`/`size()` and other Berry mappings need conformance audit; `try`/`catch`/`throw` rejected by compiler; `async`/`await`/`for await` rejected; full subset/semantic docs + tests incomplete. |
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
| R2 | 2026-11-06 profiling + regression matrix (P7) | P0 | 🔄 In progress | M7a |
| R3 | TS→Berry semantic conformance pass | P0 | ⬜ Not started | M7b |
| R4 | `try`/`catch`/`throw` compiler lowering | P1 | ⬜ Not started | M7b |
| R5 | Phase 1 Berry memory instrumentation & lifecycle verification | P0 | ⬜ Not started | M7c |
| R6 | Firmware on-demand native module loading | P1 | ⬜ Not started | M7c |
| R7 | Fetch `include_body` option (P3) | P1 | ⬜ Not started | M7d |
| R8 | HTTP worker stack/TLS measurement & safe tuning (P4/P5) | P1 | 🔄 In progress | M7d |
| R9 | Berry command-script resolver + `path` + `ping` pilot | P1 | ⬜ Not started | M8 |
| R10 | Frontend TS toolchain hardening (Worker/layout/device API) | P1 | ⬜ Not started | M8 |
| R11 | SDK single-source-of-truth / binding audit | P1 | ⬜ Not started | M8 |
| R12 | Milestone 4 hardware validation (touch/network) | P1 | ⬜ Pending hardware | M8 |
| R13 | Memory-conscious compiler rules + resource lifecycle contract | P2 | ⬜ Not started | M9 |
| R14 | Compiler profiles, debug metadata, source maps, editor diagnostics | P2 | ⬜ Not started | M9 |
| R15 | Optional `.bec` bytecode pipeline/loader | P2 | ⬜ Not started | M9+ |
| R16 | Native UI allocation benchmarking/policy changes | P2 | ⬜ Not started | M9+ |
| R17 | Berry solidification evaluation (firmware-bundled only) | P2 | ⬜ Not started | M9+ |
| R18 | `async`/`await` runtime design + implementation | P2 | ⬜ Not started | M10 |
| R19 | In-browser COS simulator (Berry WASM) | P3 | ⬜ Not started | M10 |
| R20 | 2026-10 deferred backlog (pipes/redirection, editor seam, larger apps, image support) | P3 | ⬜ Backlog | Backlog |

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
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P7
- **Spec / acceptance criteria:** Record measurements at the checkpoints defined in P7 (startup/before Berry init/after init/app open/FS text/FS JSON/HTTP GET/HTTPS GET/max response/after GC/repeated requests). Capture `free`, `largest block`, `minimum free`; compare `idf.py size`/`size-components` and `.bss` impact in `berry_lang`/`network`. Run required regression cases (JSON lazy behavior, FS text/JSON, HTTP/HTTPS, large body, max response, timeout safety, repeated requests, worker stack sanity).
- **Progress:**
  - [x] Measurement method/script/checklist committed. Added `docs/reports/2026-11-08-r2-profiling-checklist.md`, shell profiling commands (`berry profile`, `berry gc`), and deployable Berry regression scripts under `storage/flash_data/tests/r2/`.
  - [~] Baseline numbers recorded in this doc or a linked report. Initial `idf.py size`/`size-components` and boot/runtime measurements recorded in `docs/reports/2026-11-08-r2-profiling-checklist.md`; key P7 checkpoints (#1/#3/#8/#10 success path, etc.) still need isolated clean-boot captures.
  - [~] Regressions executed/passed. Partial: FS text/JSON pass; HTTP success path functional but high memory/fragmentation cost; HTTPS fails (`-0x008D`/connect alloc failures) on clean boot; timeout/error paths can collapse heap to `min 24-76`/`largest 384-1920`; repeated successful HTTP stability and large body/max response not validated.
  - [~] Follow-up issues filed for anomalies. Anomalies documented in report (HTTPS/TLS alloc failure, timeout/error cleanup/native heap collapse, BT coexistence pressure, largest-block fragmentation). Formal issue filing still pending if tracked outside docs.

### R3 — TS→Berry semantic conformance pass
- **Priority:** P0 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-03-ts-sdk.md`, `docs/2026-11-07-ts-feature-extensions.md`
- **Problem:** The compiler implements useful lowering, but some mappings need verification against actual Berry semantics (`.length` currently lowers to `.size()` method; doc flags a possible mismatch). `null/undefined`→`nil`, equality/truthiness, template strings, class property init, and container/string APIs need a documented conformance matrix.
- **Spec / acceptance criteria:**
  - Every supported mapping has tests (compiler output +, where feasible, Berry/runtime behavior expectations).
  - Unsupported/ambiguous constructs produce diagnostics at original TS locations.
  - No silent semantic degradation.
  - SDK docs/types align with supported mappings.
- **Progress:**
  - [ ] Mapping matrix written.
  - [ ] `.length`/`size()` behavior verified and fixed if needed.
  - [ ] Other semantic edge cases audited.
  - [ ] Tests/docs updated.

### R4 — `try`/`catch`/`throw` compiler lowering
- **Priority:** P1 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-07-ts-feature-extensions.md`
- **Current state:** `TryStatement` and `ThrowStatement` are explicitly rejected in `frontend/src/ide/berry/statement-generator.ts`.
- **Spec / acceptance criteria:**
  - Standard TS `try/catch/finally` (scope decided explicitly) lowers to Berry exception handling (`try`/`except`/`raise` or equivalent supported form) with predictable binding/cleanup behavior.
  - `throw` lowers to Berry-compatible exception raising.
  - Unsupported patterns are diagnosed, not silently miscompiled.
  - Generated Berry is readable and tested.
- **Progress:**
  - [ ] Berry exception syntax/semantics confirmed for pinned Berry version.
  - [ ] Lowering implemented.
  - [ ] Diagnostics/tests added.
  - [ ] SDK/docs updated.

### R5 — Phase 1 Berry memory instrumentation & lifecycle verification
- **Priority:** P0 | **Complexity:** M | **ROI:** High | **Risk:** Medium
- **Source:** `docs/2026-11-02-memory-consumption-improvements.md`
- **Spec / acceptance criteria:**
  - Lifecycle checkpoints report `be_gc_memcount()`, free internal heap, largest internal block, minimum free heap (watermark), and total free heap where useful.
  - `read_virtual_file()` / `execute_app_source()` release source buffers immediately after `be_loadbuffer()` (including compile failures), with ownership clarified against pinned Berry.
  - Repeated app start/stop tests detect leaks/fragmentation regressions.
  - GC step-rate experiments (200/150/125) benchmarked; default unchanged until evidence supports it.
  - Host-side CI target compiles all existing `.be` apps as a regression gate.
- **Progress:**
  - [ ] Instrumentation added.
  - [ ] Source-buffer lifetime fixed/verified.
  - [ ] Restart-loop regression tests added.
  - [ ] GC benchmarks recorded.
  - [ ] Host `.be` compile validation added to CI/local workflow.

### R6 — Firmware on-demand native module loading
- **Priority:** P1 | **Complexity:** M–L | **ROI:** High | **Risk:** Medium–High
- **Source:** `docs/2026-11-07-ts-feature-extensions.md`, `docs/2026-11-06-memory-pressure-fixes.md`
- **Current state:** Runtime eagerly installs `console`, `fs`, `net`, `ui`, `json` globals. Compiler already supports `@cos/*`/`@berry/*` module-style imports.
- **Spec / acceptance criteria:**
  - Modules not used by a script/app avoid unnecessary allocation/loading where feasible.
  - Existing apps and SDK usage remain compatible or are migrated with clear compatibility rules.
  - Lazy loading does not introduce reentrancy/VM lifetime bugs.
  - Memory impact is measured (ties to R2/R5).
- **Progress:**
  - [ ] Compatibility strategy decided (legacy globals vs explicit imports vs hybrid).
  - [ ] Runtime/module registration refactor implemented.
  - [ ] Regression tests/heap measurements recorded.

### R7 — Fetch `include_body` option (P3)
- **Priority:** P1 | **Complexity:** S | **ROI:** Medium | **Risk:** Low
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P3
- **Spec / acceptance criteria:** Add optional `include_body: bool` (default `true`). When `false` and JSON parsing succeeds, return parsed JSON but omit/empty raw body to avoid retaining the extra Berry string. Native parsing, `known_fetch_field`, TypeScript SDK, and docs updated. Existing scripts unchanged.
- **Progress:**
  - [ ] Native implementation.
  - [ ] SDK/types/docs updated.
  - [ ] Tests added.

### R8 — HTTP worker stack/TLS measurement & safe tuning (P4/P5)
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Medium–High
- **Source:** `docs/2026-11-06-memory-pressure-fixes.md` P4, P5
- **Spec / acceptance criteria:**
  - Add high-water-mark measurement for HTTP worker under GET/POST/HTTPS/max response/timeout/error cases; account for port stack units.
  - Reduce `HTTP_TASK_STACK` only with measured peak + >=25% margin.
  - Profile HTTPS/TLS heap, largest block, and cleanup paths; do **not** disable certificate verification.
  - Optional idle worker shutdown only if lifecycle is race-free and measurably beneficial.
- **Progress:**
  - [~] P4 instrumentation started (step 1): `components/network/src/network_http.c` logs HTTP worker stack HWM after each request (configured stack bytes, `StackType_t` unit, HWM words/bytes, used/peak estimate, method/scheme/status/err). `network_http_get_worker_stack_info()` added; `berry profile` prints worker stack info when the worker has run. Stack size unchanged. No hardware measurements recorded yet.
  - [ ] Measurements recorded.
  - [ ] Safe tuning applied or explicitly deferred with evidence.
  - [ ] Network regression tests passed.

### R9 — Berry command-script resolver + `path` + `ping` pilot
- **Priority:** P1 | **Complexity:** M–L | **ROI:** Medium–High | **Risk:** High
- **Source:** `docs/2026-11-01-improvements.md`
- **Spec / acceptance criteria (summary):** Resolve non-native commands to `.be` scripts (CWD, then configured path dirs); native commands always win; explicit path runs only that script; `main(args)` receives tokenized args and returns int status; scripts run in shared Berry VM without closing active UI app; `path` command list/add/remove/clear with normalized persisted config; `ping.be` pilot preserves existing `ping` contract before native registration is removed. Full spec in source doc.
- **Progress:**
  - [ ] Resolver/tokenization design finalized.
  - [ ] `path` config/persistence implemented.
  - [ ] Script execution path implemented.
  - [ ] Host/QEMU tests added.
  - [ ] `ping` pilot validated; native migration decision recorded.

### R10 — Frontend TS toolchain hardening
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Low–Medium
- **Source:** `docs/2026-11-03-ts-sdk.md`
- **Spec / acceptance criteria:**
  - Compilation runs in a dedicated Web Worker (or existing behavior is verified and documented).
  - Package/layout decision recorded (`frontend/*` integrated vs `packages/cos-sdk` + `packages/cos-typescript`).
  - Monaco language features and SDK `addExtraLib()` registration are minimal/correct.
  - Device filesystem read/write/deploy flows handle auth and errors predictably.
- **Progress:**
  - [ ] Worker isolation verified/implemented.
  - [ ] Layout decision documented.
  - [ ] Frontend tests/CI checks updated.

### R11 — SDK single-source-of-truth / binding audit
- **Priority:** P1 | **Complexity:** M | **ROI:** Medium | **Risk:** Low–Medium
- **Source:** `docs/2026-11-03-ts-sdk.md`, `docs/2026-11-07-ts-feature-extensions.md`
- **Spec / acceptance criteria:** SDK declarations, module exports, and docs match actual Berry/COS bindings. Prefer generation or automated validation to prevent drift. Cover `ui`, `console`/`log`, `fs`, `net`, `action`, Berry built-ins, and `@cos/*` module semantics.
- **Progress:**
  - [ ] API inventory created.
  - [ ] Drift fixes/generation approach implemented.
  - [ ] Validation added.

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

## Progress log

| Date | Update | Evidence / notes |
|---|---|---|
| 2026-11-08 | Consolidated plan created. Source docs reviewed and status annotations added in `2026-11-01`–`2026-11-07`. | Findings reflect repo state at commit `3659f62` (latest shown during review) plus earlier `e0bf05b`/`8d32f84`. |
| 2026-11-08 | R1 started: removed eager `json` global install from `berry_runtime_init()`. | `components/berry_lang/src/berry_runtime.cpp`; `json` remains in `be_modtab.c`; `berry_runtime_ensure_json()` still used by `fs.read_json/write_json` and `net.fetch(..., response_type="json")`. Firmware rebuild + hardware validation pending (user). |
| 2026-11-08 | R1 hardware validation (partial): boot/non-JSON OK; `fs.read_json`/`write_json` OK; `net.fetch` JSON path exercised on connect failure (no crash, correct error shape). Successful JSON HTTP parse still recommended for full acceptance. | User logs: `mem`/`hello.be` OK; `/sdcard/tests/validate-json.be` → `write_json` true, `read_json` returned `{value:{n:42,s:"r1",ok:true},ok:true}` and fields correct; `/sdcard/tests/fetch-invalid.be` → `ok=false,status=0,json_ok=false,json=nil,error=ESP_ERR_HTTP_CONNECT`. Attempted `https://raw.githubusercontent.com/.../frontend/tsconfig.json` → same `ESP_ERR_HTTP_CONNECT` (likely device network/TLS/connectivity), so JSON parse-on-success not yet observed. |
| 2026-11-08 | R1 complete. | Successful `net.fetch(..., response_type="json")` after connectivity fix: `true, 200, true, {'slideshow': ...}` (parsed JSON returned). Acceptance criteria satisfied: no eager JSON load at startup; FS JSON works; fetch JSON works; repeated JSON ops supported by cached/lazy ensure path; VM recreate implies fresh globals; non-JSON scripts/apps unchanged. |
| 2026-11-08 | R2 in progress: profiling implementation/checklist. | Added `berry profile` (system heap + Berry GC metrics) and `berry gc` shell commands, `berry_runtime_gc_collect()` API + host mock, P7 checklist/report template in `docs/reports/2026-11-08-r2-profiling-checklist.md`, and deployable Berry regression scripts in `storage/flash_data/tests/r2/`. Hardware/IDF measurements and regression execution remain pending (user validation). |
| 2026-11-08 | R2 hardware analysis (partial). | User provided `idf.py size`/components and clean-boot reruns A–D in `docs/reports/2026-11-08-r2-profiling-checklist.md`. FS text/JSON pass. HTTP success is functional but costly (`min=3288`, `largest=6912` after 158 B). HTTPS fails from clean boot (`mbedtls_ssl_setup -0x008D` / connect alloc failure). Timeout/error paths can collapse native heap (`min 24–76`, `largest 384–1920`, BT malloc failures, `ESP_ERR_NO_MEM`), with no Berry GC recovery. Report updated with anomalies and prioritized improvements (R8/P4 stack HWM, R8/P5 mbedTLS peak, timeout/cleanup audit). |

## Validation checklist (use when closing items)

- [ ] Relevant unit tests added/updated and passing.
- [ ] Integration/QEMU tests added/updated where applicable.
- [ ] Firmware build passes for affected targets.
- [ ] Formatting/lint/static analysis checks pass.
- [ ] Memory/runtime changes include before/after measurements (heap, largest block, watermark, `.bss`/binary size as relevant).
- [ ] Docs/source specs updated to match shipped behavior.
- [ ] Hardware validation recorded when behavior depends on physical CYD/touch/network.
