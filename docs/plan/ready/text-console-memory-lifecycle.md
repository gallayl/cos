# Text console memory and lifecycle optimization

- **ID:** text-console-memory-lifecycle
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Consolidated plan: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (item line ~50). Related: [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md), [`./boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) (recovery/availability assumptions), [`./service-memory-budget-observability.md`](./service-memory-budget-observability.md) (`svc_mem` console fields), Berry UI/display ownership and console visibility integration points. Code evidence: `components/text_console/src/text_console.cpp:28-34,364-427,429-468,504-548,615-642`, `components/text_console/src/text_buffer.h:9,54-71,105-148`, `components/text_console/src/text_buffer.c:88-124,498-509,607-611`.
- **Approval:** User approved in chat on 2026-11-16 ("1A,2A,3A,4A,5A,6A,7A,8: Approve"). **Explicitly pre-approved for implementation and PR flow** (merge remains human). Open questions: **none**.

## Summary
The text console keeps a large static character grid buffer resident, runs a render task, and can lazily allocate heap scrollback history. This item reduces **both peak and steady-state** console memory/lifecycle cost where safe by releasing scrollback on display-ownership transitions, suspending render/touch work while inactive while preserving UART/listener forwarding and buffered console state, and documenting/measuring costs and behavior—without changing max grid/scrollback compile-time limits in this scope.

## Decisions (clarified 2026-11-16)
1. **Memory goal (A):** optimize both peak and steady-state memory.
2. **Hidden/app-owned behavior (A):** when console UI is hidden or display is app-owned (`text_console_set_visible(false)` / equivalent ownership transition), **keep UART/stdout forwarding and output listener callbacks active and continue updating console buffer/history** so output is preserved on return; **stop/suspend rendering and touch handling** while hidden/inactive. (Intentional behavior change vs current skip-buffer-writes-when-hidden.)
3. **Scrollback auto-release (A):** **auto-free heap scrollback on app/display ownership transition / hide** (in addition to existing/manual release mechanisms such as `text_console_release_scrollback()`), with safe handling of alloc-failure/disabled-history state.
4. **Render/task/base grid lifecycle (A):** **suspend render task/touch handling when inactive; keep static base grid and task/resources resident** (no deinit/reinit or lazy base-grid allocation in this scope) to limit race/complexity risk.
5. **Dimensions/limits (A):** **keep current** `TEXT_BUF_MAX_ROWS`, `TEXT_BUF_MAX_COLS`, and `TEXT_BUF_SCROLLBACK_ROWS`; optimize lifecycle/retention behavior only.
6. **Minimum low-memory/inactive capabilities (A):** **UART/stdout forwarding, output listener callbacks, and console command/log integration must remain intact**. Display rendering/history presentation may be suspended/reduced per state, but console APIs must remain safe and behavior must be documented.
7. **Validation/acceptance (A):** source-based memory-cost documentation + automated checks per `AGENTS.md`/scope, plus required hardware/manual measurements and behavior evidence (see Validation plan).

## Scope
- **Document current memory costs** (source-based, with measurements where feasible/agreed): static grid/buffer footprint, scrollback heap (lazy, max), render task stack/prio, mutex/state, and key runtime structures.
- **Lifecycle/visibility behavior:** while hidden/app-owned, preserve stdout hook/UART forwarding and listener delivery; keep console text buffer/history updated; suspend rendering/touch and avoid unnecessary render wakeups/races.
- **Scrollback policy:** auto-release scrollback heap on hide/display-ownership transition; ensure `text_console_release_scrollback()` and clear/reinit paths remain correct; handle allocation-failure-disabled state safely and document behavior.
- **Inactive rendering optimization:** suspend/idle render task work and touch handling while inactive/hidden (base resources remain resident). No base grid/task deinit/reinit or compile-time limit changes.
- **Correctness preservation:** maintain stdout hook safety, UART/log output, output listener behavior, console commands/integration, resize/visibility/cursor/size/write/clear APIs, and documented scrollback semantics.

## Non-goals
- Redesigning console UX/features unrelated to memory/lifecycle.
- Changing font rendering or display driver behavior except required integration points.
- Reducing/tuning `TEXT_BUF_MAX_ROWS/COLS` or `TEXT_BUF_SCROLLBACK_ROWS`.
- Full console deinit/reinit or lazy allocation of static base grid/render task resources.
- Removing scrollback capability entirely.
- Broader unified-service-lifecycle implementation beyond console-scoped integration/behavior needed here.

## Acceptance criteria
- [ ] Current console memory costs are documented with source-based evidence and runtime measurements as available (static grid/buffer, scrollback heap, render task stack, mutex/state/overheads).
- [ ] On hide/app/display-ownership transition, scrollback heap is auto-released per policy, and UART/stdout forwarding + listener callbacks continue while console buffer/history remains preserved for return (with rendering/touch suspended).
- [ ] Render task/touch handling is suspended/inactive when console is hidden/inactive without breaking correctness, races, or stdout/logging; base grid/task resources remain resident and limits unchanged.
- [ ] Console/UART output correctness, output listener behavior, existing commands/APIs, resize/visibility, and scrollback release/restore semantics are preserved and documented per clarified scope.
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (or blockers reported with actionable details); required hardware/manual evidence is captured.

## Validation plan
- **Automated (firmware scope, run/report per `AGENTS.md`):** `make format-check`, `make test` (required). `make build` and `make lint` run as feasible for firmware changes in an ESP-IDF v6.1 environment; if `make lint` cannot run, report as best-effort blocker with concrete setup/rerun steps (do not claim pass).
- **Hardware/manual (required for this item):** on-device evidence for (1) heap/watermark/task/stack impact before/after or across transitions as feasible, (2) hide/show and app/display-ownership transitions including scrollback auto-release and state/behavior on return, (3) UART/log output and output listener correctness while hidden/inactive, (4) rendering/touch suspended while hidden and restored correctly when visible, (5) no regressions in existing console commands/APIs and resize/clear behavior. Record exact steps/measurements in progress/evidence or linked report.
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
- Implementation/validation: not started.
