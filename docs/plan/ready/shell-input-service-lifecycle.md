# Shell input service lifecycle and memory bounds

- **ID:** shell-input-service-lifecycle
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Updated:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `components/shell/src/shell.c:167-220`, `components/shell/src/shell_input.c:19-52`, `components/shell/src/shell_input.c:951-988`. Related: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md), [`docs/plan/ready/unified-service-lifecycle.md`](./unified-service-lifecycle.md) (shell core/always-on), [`docs/plan/ready/boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) (serial shell recovery in all v1 profiles), [`docs/plan/needs-clarification/text-console-memory-lifecycle.md`](../needs-clarification/text-console-memory-lifecycle.md) (console integration context). Code evidence: `components/shell/src/shell_input.c` (`INPUT_LINE_MAX` 128, `HISTORY_SIZE` 8, `INPUT_STREAM_SIZE` 256, `RX_TASK_STACK` 2048/`uart_rx`, `INPUT_TASK_STACK` 8192/`shell_in`), `components/shell/src/shell_input.h`, `components/shell/include/shell.h`, `components/shell/src/shell.c`, `test/test_shell_input.c`.
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-16 ("Approve and move to ready"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
The shell/input subsystem runs always-on tasks and keeps fixed static buffers for line editing and history. Shell is a local recovery path in current ready specs, so this item treats interactive shell input as **always-on** in all supported modes/profiles and focuses on documenting/measuring footprint and applying safe, evidence-backed optimization (notably reducing `shell_in` stack from 8192 to 4096) without adding stop/suspend/gating semantics.

## Scope
- **Always-on lifecycle (explicit):** keep shell input initialized and available in all supported modes/profiles, aligned with `unified-service-lifecycle` (core/always-on) and `boot-profiles-feature-modes` (serial/on-device recovery in all v1 profiles). No stop/suspend/gating behavior is introduced.
- **Console ownership/routing:** when Berry app/script or another component owns console interaction (line/key/char input providers, text console/UI ownership), shell input infrastructure remains alive and hand-off continues via existing provider/cancel/feed mechanisms (`shell_feed_input`, console input provider registration/cancel, `shell_is_reading_line_input` semantics) with no lifecycle change.
- **Footprint documentation:** document static memory/task footprint for shell input (tasks/stacks, static buffers, stream buffer, ownership state) and record runtime evidence as agreed.
- **Numeric changes (approved by user):**
  - Reduce `shell_in` task stack (`INPUT_TASK_STACK`) from **8192** to **4096**.
  - Keep unchanged: `RX_TASK_STACK` 2048 (`uart_rx`), `INPUT_LINE_MAX` 128, `HISTORY_SIZE` 8 (`8 * 128` history storage), `INPUT_STREAM_SIZE` 256.
- **Evidence/safety margin:** apply/keep the `shell_in` 4096 reduction only when runtime + static evidence demonstrates a documented safety margin in agreed scenarios (see Validation plan). If evidence does not support 4096, implementation must restore/retain a safe value and document findings (do not ship an unsafe reduction).
- Preserve command execution, history, line editing, prompt behavior, and script/console/Bluetooth-keyboard/WebSocket-fed input integration behavior.

## Non-goals
- Shell UX redesign unrelated to memory/lifecycle.
- Command/parser feature additions.
- Broad refactors of all shell commands.
- Disabling, suspending, or profile-gating shell input in recovery-required modes/profiles.
- Changing `uart_rx` stack, `INPUT_LINE_MAX`, `HISTORY_SIZE`, or `INPUT_STREAM_SIZE` in this item.
- Adding new user-facing diagnostics/reporting commands or telemetry surfaces unless required later by separate clarification (not in scope for this item).

## Acceptance criteria
- [ ] Static shell input footprint is documented (tasks/stacks, static buffers/history/line/stream sizes, key ownership/state) with source references.
- [ ] Always-on lifecycle and "stay alive, keep routing" ownership behavior are explicitly documented and preserved; no stop/suspend/gating semantics are introduced.
- [ ] `shell_in` task stack is set to 4096 and other specified limits remain unchanged **only when** runtime + static evidence in agreed scenarios demonstrates a documented safety margin; otherwise the stack value is kept/restored to a safe level with evidence recorded.
- [ ] Interactive shell behavior remains correct for supported modes (UART/stdin transport, `shell_feed_input` paths such as Bluetooth keyboard/WebSocket console input where applicable, line editing/history, prompt, and Berry console input provider hand-off/cancel semantics).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass and are reported with evidence.

## Validation plan
- **Automated (required, affected scope):** `make format-check`, `make test`. For firmware changes run `make build` and `make lint` when ESP-IDF v6.1 / clang-tidy environment is available; report exact commands, pass/fail, and blockers per `AGENTS.md`.
- **Runtime + static evidence (required for stack reduction):** capture/document static accounting (before/after) and on-device runtime observations in agreed scenarios (e.g. normal UART shell interaction, history/line-edit use, Bluetooth-keyboard and/or WebSocket-fed input where feasible, Berry console input/ownership hand-off). Evidence must include `uart_rx` and `shell_in` stack high-water marks and relevant heap/stream behavior sufficient to justify a safety margin for `shell_in` at 4096 (or justify keeping/restoring a larger value). Record method, scenarios, and results.
- **Hardware/manual (required when feasible):** interactive UART/Bluetooth/remote input behavior and long-session stability checks consistent with scenarios above. If hardware/tools are unavailable, do not claim pass; report concrete blockers and what remains.

## Expected areas/files
- `components/shell/src/shell_input.c` (stack constant/footprint-related changes and evidence-driven adjustments)
- `components/shell/src/shell_input.h`, `components/shell/include/shell.h` (only if API/docs/comments need alignment)
- `components/shell/src/shell.c` (integration points only if required; expected minimal/none)
- `test/test_shell_input.c` and/or shell-related mocks/tests if behavior/assumptions change
- Docs: this item and cross-links in `docs/plan/consolidated-improvement-plan.md` when status moves

## Dependencies / prerequisites
- Align with [`docs/plan/ready/unified-service-lifecycle.md`](./unified-service-lifecycle.md) and [`docs/plan/ready/boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) (shell remains core/recoverable; no lifecycle gating in this item).
- Coordinate/consider interactions with text console ownership, Bluetooth HID input routing, WebSocket console feed path, and Berry console input providers (`berry_runtime_set_console_input_provider`).
- On-device access/tooling for stack high-water/heap observation is needed to fully satisfy runtime evidence (otherwise a documented blocker remains for the reduction).

## Risks / constraints
- Reducing `shell_in` stack without sufficient margin can cause hard-to-debug crashes, corruption, or input loss under deeper command/script/console-interaction paths.
- Input tasks handle asynchronous sources (`uart_rx`, stream buffer, `shell_feed_input`); timing-sensitive behavior must not regress.
- Recovery/debug availability must not regress; always-on behavior is mandatory.
- ESP32 RAM/stack constraints and environment-dependent call depth mean evidence must be scenario-based and conservative.

## Open questions
- None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify-feature, agent): loaded `clarify-feature` skill; reviewed `docs/README.md`, target item, consolidated plan, `components/shell/src/shell_input.c`, `components/shell/src/shell.c`, `components/shell/src/shell_input.h`, `components/shell/include/shell.h`, `test/test_shell_input.c`, and ready items `unified-service-lifecycle`/`boot-profiles-feature-modes`. Identified gaps: lifecycle intent vs recovery constraints, numeric limit policy/evidence bar, console-ownership behavior, and diagnostics scope.
- 2026-11-16 (clarify-feature, user): **Always-on, optimize only** — shell input remains initialized/available in all supported modes/profiles (no stop/suspend/gating).
- 2026-11-16 (clarify-feature, user): **Numeric limits** — reduce `shell_in` stack to 4096; keep `uart_rx` (2048), `INPUT_LINE_MAX` (128), `HISTORY_SIZE` (8), `INPUT_STREAM_SIZE` (256) unchanged.
- 2026-11-16 (clarify-feature, user): **Console ownership** — stay alive, keep routing via existing provider/cancel/feed mechanisms.
- 2026-11-16 (clarify-feature, user): **Diagnostics/evidence** — runtime + static evidence required; 4096 reduction accepted only with demonstrated safety margin in agreed scenarios.
- 2026-11-16 (clarify-feature, agent): drafted complete spec satisfying Ready-contract fields (scope/non-goals/acceptance/validation/areas/deps/risks, open questions none).
- 2026-11-16 (clarify-feature, user): explicitly approved spec and pre-approved implementation/PR flow; marked Ready and moved to `docs/plan/ready/shell-input-service-lifecycle.md`. No implementation/validation run.