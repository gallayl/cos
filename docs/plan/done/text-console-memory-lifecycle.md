# Text console memory and lifecycle optimization

- **ID:** text-console-memory-lifecycle
- **Status:** done (delivered; PR CI green, hardware/manual validation pending, merge remains human)
- **Created:** 2026-11-16
- **Completed:** 2026-11-17
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Consolidated plan: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (item line ~50). PR: [#46 feat(text-console): reduce hidden-state memory and lifecycle cost](https://github.com/gallayl/cos/pull/46) (ready for review). Related: [`../ready/unified-service-lifecycle.md`](../ready/unified-service-lifecycle.md), [`../ready/boot-profiles-feature-modes.md`](../ready/boot-profiles-feature-modes.md) (recovery/availability assumptions), [`../ready/service-memory-budget-observability.md`](../ready/service-memory-budget-observability.md) (`svc_mem` console fields), Berry UI/display ownership and console visibility integration points. Code evidence: `components/text_console/src/text_console.cpp:28-34,364-427,429-468,504-548,615-642`, `components/text_console/src/text_buffer.h:9,54-71,105-148`, `components/text_console/src/text_buffer.c:88-124,498-509,607-611`.
- **Approval:** User approved in chat on 2026-11-16 ("1A,2A,3A,4A,5A,6A,7A,8: Approve"). **Explicitly pre-approved for implementation and PR flow** (merge remains human). Open questions: **none**.

## Summary
The text console keeps a large static character grid buffer resident, runs a render task, and can lazily allocate heap scrollback history. This item reduces **both peak and steady-state** console memory/lifecycle cost where safe by releasing scrollback on display-ownership transitions, suspending render/touch work while inactive while preserving UART/listener forwarding and buffered console state, and documenting/measuring costs and behavior—without changing max grid/scrollback compile-time limits in this scope.

## Decisions (clarified 2026-11-16)
1. **Memory goal (A):** optimize both peak and steady-state memory.
2. **Hidden/app-owned behavior (A):** when console UI is hidden or display is app-owned (`text_console_set_visible(false)` / equivalent ownership transition), **keep UART/stdout forwarding and output listener callbacks active and continue updating the resident base grid/console buffer** while inactive; **stop/suspend rendering and touch handling**. Output/log capture continues, but heap scrollback is not retained across hide (see decision 3). (Intentional behavior change vs current skip-buffer-writes-when-hidden.)
3. **Scrollback auto-release (A):** **auto-free heap scrollback on app/display ownership transition / hide** (in addition to existing/manual release mechanisms such as `text_console_release_scrollback()`), with safe handling of alloc-failure/disabled-history state. **Scrollback history beyond the base grid is discarded on hide and is not preserved on return** (it may be reallocated later per existing/new safe behavior; base grid/current buffer contents remain subject to ongoing writes while inactive).
4. **Render/task/base grid lifecycle (A):** **suspend render task/touch handling when inactive; keep static base grid and task/resources resident** (no deinit/reinit or lazy base-grid allocation in this scope) to limit race/complexity risk.
5. **Dimensions/limits (A):** **keep current** `TEXT_BUF_MAX_ROWS`, `TEXT_BUF_MAX_COLS`, and `TEXT_BUF_SCROLLBACK_ROWS`; optimize lifecycle/retention behavior only.
6. **Minimum low-memory/inactive capabilities (A):** **UART/stdout forwarding, output listener callbacks, and console command/log integration must remain intact**. Display rendering/history presentation may be suspended/reduced per state, but console APIs must remain safe and behavior must be documented.
7. **Validation/acceptance (A):** source-based memory-cost documentation + automated checks per `AGENTS.md`/scope, plus required hardware/manual measurements and behavior evidence (see Validation plan).

## Scope
- **Document current memory costs** (source-based, with measurements where feasible/agreed): static grid/buffer footprint, scrollback heap (lazy, max), render task stack/prio, mutex/state, and key runtime structures.
- **Lifecycle/visibility behavior:** while hidden/app-owned, preserve stdout hook/UART forwarding and listener delivery; keep the resident base grid/console buffer updated; suspend rendering/touch and avoid unnecessary render wakeups/races. Heap scrollback is not retained across hide (see scrollback policy).
- **Scrollback policy:** auto-release scrollback heap on hide/display-ownership transition (scrollback history beyond the base grid discarded, not preserved on return); ensure `text_console_release_scrollback()` and clear/reinit/reallocation paths remain correct; handle allocation-failure-disabled state safely and document behavior.
- **Inactive rendering optimization:** suspend/idle render task work and touch handling while inactive/hidden (base resources remain resident). No base grid/task deinit/reinit or compile-time limit changes.
- **Correctness preservation:** maintain stdout hook safety, UART/log output, output listener behavior, console commands/integration, resize/visibility/cursor/size/write/clear APIs, and documented buffer/scrollback semantics.

## Non-goals
- Redesigning console UX/features unrelated to memory/lifecycle.
- Changing font rendering or display driver behavior except required integration points.
- Reducing/tuning `TEXT_BUF_MAX_ROWS/COLS` or `TEXT_BUF_SCROLLBACK_ROWS`.
- Full console deinit/reinit or lazy allocation of static base grid/render task resources.
- Removing scrollback capability entirely.
- Broader unified-service-lifecycle implementation beyond console-scoped integration/behavior needed here.

## Acceptance criteria
- [x] Current console memory costs are documented with source-based evidence (static grid/buffer, scrollback heap, render task stack, mutex/state/overheads). **Runtime measurements/hardware evidence pending** (on-device CYD/ESP32).
- [x] On hide/app/display-ownership transition, scrollback heap is auto-released per policy (scrollback history beyond the base grid discarded, not preserved on return), and UART/stdout forwarding + listener callbacks continue while the resident base grid/console buffer continues to be updated (with rendering/touch suspended). **On-device behavior evidence pending.**
- [x] Render task/touch handling is suspended/inactive when console is hidden/inactive (task waits while hidden; base grid/task resources remain resident, limits unchanged). **On-device correctness/race validation pending.**
- [x] Console/UART output/listener/API/resize/visibility/buffer-semantics behavior preserved and documented per clarified scope in implementation/docs. **On-device regression validation pending.**
- [x] Automated checks required by `AGENTS.md` for affected scope executed/reported (see evidence). Required hardware/manual evidence **not yet captured** (tracked as blocking PR comment until evidenced or explicitly waived by a human with rationale). PR CI checks green. Review/approval and merge remain human actions.

## Validation plan
- **Automated (firmware scope, run/report per `AGENTS.md`):** `make format-check`, `make test` (required). `make build` and `make lint` run as feasible for firmware changes in an ESP-IDF v6.1 environment; if `make lint` cannot run, report as best-effort blocker with concrete setup/rerun steps (do not claim pass).
- **Hardware/manual (required for this item):** on-device evidence for (1) heap/watermark/task/stack impact before/after or across transitions as feasible, (2) hide/show and app/display-ownership transitions including scrollback auto-release, scrollback discarded/not preserved on return, and base buffer/output-capture behavior on return, (3) UART/log output and output listener correctness while hidden/inactive, (4) rendering/touch suspended while hidden and restored correctly when visible, (5) no regressions in existing console commands/APIs and resize/clear behavior. Record exact steps/measurements in progress/evidence or linked report; reuse `service-memory-budget-observability` `svc_mem`/A-B checklist where useful.
- **Reporting:** report each executed command/check as pass/fail with notes; do not rely on CI.

## Expected areas/files
- `components/text_console/src/text_console.cpp`
- `components/text_console/src/text_buffer.c`
- `components/text_console/src/text_buffer.h`
- `components/text_console/include/text_console.h` (API/behavior docs as needed)
- Integration points/callers as needed: `main/main.c`, `components/shell/`, `components/berry_lang/`, Berry UI/display ownership code paths
- Documentation/evidence: this item and/or linked report under `docs/reports/` or `docs/progress/` as appropriate

## Dependencies / prerequisites
- Align with [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md) and any boot/mode profile decisions (console-scoped changes must remain compatible).
- Coordinate with Berry UI app and console visibility/display-ownership behavior (hide/show transitions).
- Hardware access (CYD/ESP32) required for mandatory validation evidence.

## Cross-plan contract (2026-11-16, clarified)
- **Source of truth:** this plan owns text-console lifecycle/behavior semantics (hidden/app-owned/inactive behavior, scrollback auto-release, render/touch suspension, recovery guarantees, and safe API behavior).
- [`./boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) must preserve the recovery/local-console guarantees defined here when console is kept/enabled in a profile.
- [`./service-memory-budget-observability.md`](./service-memory-budget-observability.md) should report console state/fields (`init/deinit`, render/task active, configured/static buffer/scrollback sizes, task stack/HWM when exposed) consistent with the semantics defined here.

## Risks / constraints
- Stdout hooking and render task synchronization are sensitive; regressions can affect logging, listener delivery, or deadlock/races.
- Auto-releasing scrollback or changing render scheduling while output is active must be mutex/task-safe and preserve buffer consistency.
- Preserving buffer/history updates while hidden (decision A2) changes behavior and may affect CPU/memory-write patterns; must be validated for correctness and performance.
- Static `s_buf` layout/size tied to max rows/cols and rendering assumptions; base grid remains resident by design in this scope.
- ESP-IDF 6.1/ESP32 resource constraints; validation may be blocked by environment/hardware availability and must be reported concretely.

## Open questions
None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus) in `docs/plan/needs-clarification/`.
- 2026-11-16 (clarification): reviewed `docs/README.md` Ready contract, consolidated plan context, and implementation evidence.
  - `components/text_console/src/text_console.cpp`: static `text_buffer_t s_buf` resident after init; render task `RENDER_TASK_STACK=3072`; stdout hooked via `fopencookie` forwarding to UART and optional output listener; buffer write/render notify currently gated by `s_screen_visible` in hook/write paths; `text_console_release_scrollback()` exists.
  - `components/text_console/src/text_buffer.{h,c}`: lazy heap scrollback allocation (`TEXT_BUF_SCROLLBACK_ROWS=128`, `TEXT_BUF_MAX_COLS=54`, `text_history_cell_t`), releasable/clearable; alloc failure disables history until clear and reports warning.
- 2026-11-16 (clarification): user decisions recorded (1A,2A,3A,4A,5A,6A,7A) and explicit approval/pre-approval for implementation & PR flow given in chat ("8: Approve"). Spec completed and item promoted to **Ready**.

## Memory costs (source-based, 2026-11-17 implementation)
- **Resident base grid/state:** `static text_buffer_t s_buf` in `components/text_console/src/text_console.cpp` holds `cells[TEXT_BUF_MAX_ROWS][TEXT_BUF_MAX_COLS]` (`40*54` `text_cell_t` entries, each `char ch + uint16_t fg + bool dirty` with padding) plus parser/cursor/viewport/state fields (`components/text_console/src/text_buffer.h`). Dimensions/limits unchanged (`TEXT_BUF_MAX_ROWS=40`, `TEXT_BUF_MAX_COLS=54`, `TEXT_BUF_SCROLLBACK_ROWS=128`).
- **Scrollback heap (lazy, releasable/suspendable):** `text_buffer_t.scrollback` allocates `sizeof(text_history_cell_t[TEXT_BUF_MAX_COLS]) * TEXT_BUF_SCROLLBACK_ROWS` on first retained scroll when not disabled/suspended (`text_buffer.c: retain_scrolled_row`), i.e. `128 * 54 * sizeof(text_history_cell_t{char,uint8_t})` bytes peak history buffer plus allocator overhead; freed on clear, manual release, alloc-failure disable handling paths, and on hide/app/display-ownership transition (`text_console_set_visible(false)` now suspends+frees scrollback; not retained/preserved across hide).
- **Render task:** `RENDER_TASK_STACK=3072` words/bytes-per-platform-config as passed to `xTaskCreate` (`text_console.cpp`, FreeRTOS stack units apply), priority `RENDER_TASK_PRIO=1`; task remains resident and waits indefinitely while hidden/inactive, wakes on visibility/notify when active.
- **Synchronization/state:** `s_mutex` (FreeRTOS mutex), stdout cookie/hook state, visibility/touch/listener/font globals in `text_console.cpp`. No compile-time grid/scrollback limit changes.
- **Runtime measurement/hardware evidence:** pending (see validation).

## Implementation progress (2026-11-17, `feat/text-console-memory-lifecycle`)
- Implemented lifecycle behavior per clarified scope:
  - `text_console_set_visible(false)` now auto-suspends+frees scrollback heap (discarded, not preserved across hide), suspends render/touch handling, and keeps UART/stdout forwarding + output listener callbacks active while continuing to update the resident base console buffer.
  - `stdout` hook and `text_console_write()` now update buffer while hidden/inactive (no render notifications while hidden); render task blocks while hidden and resumes on show.
  - Screen fill/notify paths avoided while hidden (`clear`, `resize`, visibility transitions). `text_console_release_scrollback()` remains supported; scrollback may be reallocated later when active per existing/new safe behavior.
- Files changed: `components/text_console/include/text_console.h`, `components/text_console/src/text_console.cpp`, `components/text_console/src/text_buffer.h`, `components/text_console/src/text_buffer.c`, plan doc (moved to `docs/plan/done/`).
- Validation (local, scope-appropriate): `make format` applied; `make format-check` **pass**. `make test` **pass** (CTest 26/26, Python `scripts` unittest 3/3; command reported a PowerShell stderr-record artifact after summaries showed pass/OK). `make docs-check` **not completed** in session (timed out/long-running); rerun `make docs-check`. `make build` **not run** (ESP-IDF v6.1 env unavailable); rerun after sourcing ESP-IDF v6.1: `make build`. `make lint` **not run** (missing `build/compile_commands.json`); rerun `make lint-db` in ESP-IDF/esp-clang env then `make lint`.
- PR: [#46](https://github.com/gallayl/cos/pull/46) (ready for review, branch `feat/text-console-memory-lifecycle`, commit `63c7359` + docs-finalization commit on PR). PR CI checks **green** at time of babysit (`gh pr checks 46`: Doxygen Check, ESP-IDF Build & Lint, Format Check, Unit Tests, all Integration Tests jobs passed). Blocking hardware/manual validation tracked in PR comment: https://github.com/gallayl/cos/pull/46#issuecomment-6937851823.
- **Remaining human actions:** review/approval and merge (human). On-device CYD/ESP32 hardware/manual validation still required per plan and blocking PR comment unless explicitly waived by a human with rationale. Moving to `done/` does not imply merge.
