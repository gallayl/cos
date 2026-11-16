# R20 — Pipes runtime v1 (minimal sequential buffered pipeline)

- **ID:** r20-pipes-runtime-v1
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Updated:** 2026-11-15 (clarified, explicitly approved, moved to ready)
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella), [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20, P3, in progress), [`../../reports/2026-11-10-r20-pipes-parsing-design.md`](../../reports/2026-11-10-r20-pipes-parsing-design.md), [`../../reports/2026-11-10-r20-pipes-runtime-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md) (primary proposal), [`../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md)
- **Approval:** Approved by user in chat on 2026-11-15 (explicit). Pre-approved for implementation/PR flow; merge remains human.

## Summary
Implement a minimal, safe in-process pipes runtime for COS shell as R20 sub-item (v1): sequential buffered **2-stage** pipeline `lhs | rhs` with **bounded temp-file capture (8 KiB)**, **Berry-first stdin contract** (`console.stdin()`), Berry-only `rhs` consumers, conservative diagnostics for unsupported forms, and documented limits/evidence.

## Scope
- Support exactly **`lhs | rhs`** (single pipe, 2 segments) in shell dispatch/runtime.
- **`lhs` (producer):** any native command or Berry script/command run through normal dispatch that writes stdout capturable by existing stdio/fd redirection mechanics (best-effort). No special exclusions added in v1.
- **Capture:** redirect `lhs` stdout to a **temp VFS file** (implementation-chosen writable location, unique name where feasible), run `lhs`, read captured output back up to **8192 bytes (8 KiB)**, then delete temp file and restore stdout/state on all paths. Capture targets stdio/fd stdout only (ESP-IDF logs may bypass; document limitation).
- **Overflow:** if captured output exceeds 8192 bytes, fail as handled non-zero with clear overflow diagnostic (states limit), **no silent truncation**, no `rhs` execution.
- **`rhs` (consumer):** **Berry command scripts/apps only** in v1. Provide piped input via **`console.stdin()` -> `string | nil`** (Berry module + TS SDK declaration in sync): returns captured string only during piped `rhs` execution, `nil` otherwise; runtime clears/restores after execution (including errors).
- **Execution/status:** if `lhs` executes and capture+wiring to a supported `rhs` succeed within cap, execute `rhs` and return **`rhs` status** (including when `lhs` returned non-zero but produced capturable output). On capture/overflow/wiring/I-O/setup failures, unsupported form/consumer, or dispatch-infrastructure failures preventing correct piped execution: handled non-zero diagnostic, do not run `rhs` (or stop appropriately), restore stdout and piped-input/temp state. `pipefail` deferred.
- **Unsupported in v1 (handled diagnostics, no partial/ambiguous execution):** 3+ stage pipelines; any `|` combined with redirection (`>`, `>>`, `2>`, `2>>`, `&>`, `&>>`, other forms); non-Berry `rhs` consumers. Reuse existing pipeline diagnostics where applicable and add specific diagnostics for unsupported Berry-only consumer and overflow/capture failures (exact strings unambiguous/tested).
- **Tests/docs/evidence:** host unit tests covering segment planning/gating, capture/overflow, stdin delivery/cleanup, stdout/piped-input restoration on error paths, and exit-status mapping; existing shell tests remain passing. Update docs for behavior/limits/unsupported forms/examples as appropriate and record validation evidence.

## Non-goals
- POSIX process pipes, concurrent streaming tasks/queues, or unbounded buffering.
- Generic stdin for native `esp_console` commands or native `rhs` consumers in v1.
- Multi-stage pipelines (`a | b | c`) or pipe+redirection combinations in v1.
- `pipefail`/advanced shell semantics, ESP log-capture-aware redirection, or changes to unrelated R20 sub-items (stderr/FD completion, editor seam, larger apps, image support).
- In-memory capture requirement if not chosen (temp VFS file selected for portability); broad HW matrix beyond specified minimal smoke.

## Decisions / resolved questions
- [x] **R20 prioritization (2026-11-15):** clarify Pipes runtime v1 now as a separate item; keep `r20-deferred-backlog.md` as umbrella for remaining R20 sub-items.
- [x] **Stdin/consumer contract (API/name/shape, 2026-11-15):** `console.stdin()` -> `string | nil` (Berry `console` module + TS SDK `console` declaration kept in sync). Returns captured piped input as a string only when executing as a piped `rhs`; returns `nil` otherwise. Runtime sets value for piped `rhs` execution and clears/restores after execution (including on errors). Bounded to v1 capture cap.
- [x] **v1 consumer scope (2026-11-15):** **Berry command scripts/apps only** as `rhs` consumers in v1. Native `esp_console` commands as `rhs` are unsupported in v1 and must produce a clear handled diagnostic (no partial/ambiguous execution).
- [x] **Supported/unsupported forms in v1 (2026-11-15):** support exactly **`lhs | rhs` (2 stages, single pipe)**. **3+ stage pipelines** and any combination of `|` with redirection (`>`, `>>`, `2>`, `2>>`, `&>`, `&>>` or other redirection forms) remain **unsupported/handled diagnostics** in v1.
- [x] **Capture cap and overflow behavior (2026-11-15):** **8192 bytes (8 KiB) hard cap**. On overflow: handled non-zero failure with clear diagnostic (must state overflow/limit, e.g. `pipes: input too large (max 8192 bytes)`), **no silent truncation**.
- [x] **Producer/lhs scope (2026-11-15):** `lhs` may be **any native command or Berry script/command** that runs through normal dispatch and produces stdout capturable by existing stdio/fd redirection mechanics (best-effort). No special exclusions added in v1 beyond existing dispatch/capture behavior; interactive/long-running/UI-disruptive cases are not specially supported.
- [x] **Capture mechanism (2026-11-15):** **Temp VFS file capture** (portable). Redirect `lhs` stdout to a temp file, run `lhs`, read captured output back up to 8192 bytes, then delete temp file and restore stdout/piped-input state on all paths. Implementation chooses a writable temp location/name (unique per invocation where feasible) and must guarantee cleanup on success, overflow, and error paths.
- [x] **Exit-status policy (2026-11-15):** If `lhs` executes and capture+wiring to a supported Berry `rhs` succeed within cap, execute `rhs` and return **`rhs` status** (even if `lhs` returned non-zero). Capture overflow, capture/wiring/I-O/setup failures, unsupported pipeline form/consumer, and dispatch-infrastructure failures that prevent correct piped execution are **handled non-zero with diagnostics** and must not run `rhs` / must stop as appropriate. `pipefail`-like behavior deferred. Exact behavior for `lhs` non-zero-with-captured-output is "pipe captured stdout to `rhs` and return `rhs` status" (documented).
- [x] **Hardware validation (2026-11-15):** **Minimal on-device (CYD) smoke required** for v1 validation/DoD: (1) happy path — Berry producer `|` Berry consumer script using `console.stdin()` produces expected output/status and console restored; (2) one handled failure/diagnostic case — e.g. overflow or unsupported form/consumer with correct diagnostic and console restored. Record device/storage/firmware notes and evidence in docs/reports or linked evidence.
- [x] **Diagnostics/limits (2026-11-15):** reuse existing pipeline diagnostics where applicable (`pipes: invalid syntax`, `pipes: not supported yet` for unsupported runtime forms not enabled by v1). Add/ensure specific clear handled diagnostics for: unsupported `rhs` consumer in v1 (Berry-only), overflow/capture-too-large, and capture/wiring failures. Exact strings may be finalized in implementation but must be unambiguous and tested.
- [x] **ESP log/stdout capture caveat (2026-11-15):** accepted limitation — v1 pipes only stdio/fd-captured stdout; ESP-IDF `ESP_LOGx` output may bypass capture depending on routing. Must be documented.
- [x] **Explicit approval to mark Ready/pre-approved for implementation (2026-11-15):** approved by user in chat; item moved to `docs/plan/ready/`.

## Open questions
- none

## Acceptance criteria / definition of done
- [ ] Spec approved explicitly and item moved to `docs/plan/ready/` with **open questions: none** (pre-approved for implementation/PR flow; merge remains human).
- [ ] `lhs | rhs` (exactly 2 stages) executes sequentially: `lhs` stdout (stdio/fd-captured, best-effort) captured via temp VFS file up to 8192 bytes and delivered to Berry `rhs` through `console.stdin()` (`string | nil`, set only during piped `rhs`, cleared/restored after).
- [ ] On successful capture+wiring to supported Berry `rhs`, `rhs` runs and pipeline returns `rhs` status (including when `lhs` returned non-zero with captured output).
- [ ] Overflow (>8192 bytes), capture/wiring/I-O/setup failures, unsupported forms (3+ stages, pipe+redirection), and non-Berry `rhs` consumers produce clear handled non-zero diagnostics; no partial/ambiguous execution; `rhs` not run when piping cannot be completed correctly. Stdout, piped-input state, and temp capture file restored/cleaned up on all paths.
- [ ] Berry `console.stdin()` native binding and TS SDK declaration updated in sync and documented (including `nil` when not piped and capture limitations).
- [ ] Host unit tests cover: v1 gating/segment planning, capture and 8 KiB overflow, stdin delivery/cleanup, stdout/piped-input/temp-file restoration on error paths, exit-status mapping (including `rhs`-status-on-success rule), and unsupported-form/consumer diagnostics. Existing shell/Berry/related tests remain passing.
- [ ] User-facing/docs updated as needed (shell behavior, limits, unsupported forms, `console.stdin()` usage, stdout-only/ESP-log caveat).
- [ ] Validation evidence recorded: `make format-check`, `make test`; firmware `make build` and `make lint` when feasible per AGENTS for touched C/C++/shell/Berry areas; other scope checks as applicable. **Minimal on-device (CYD) smoke** is required when hardware/environment is available: (1) happy path Berry producer | Berry consumer using `console.stdin()` with expected output/status and console restored; (2) one handled failure/diagnostic case (overflow or unsupported form/consumer) with correct diagnostic and console restored. If unavailable, record concrete blockers and unverified items per `AGENTS.md` (do not claim pass). Device/storage/firmware notes included when run.

## Validation plan
- **Automated (required):** `make format-check`, `make test`. For firmware/shell/Berry changes: `make build` and `make lint` when feasible (ESP-IDF v6.1 env required; report blockers with actionable setup if unavailable). Run `make docs-check` if Doxygen/public headers/docs comments in scope.
- **Hardware/manual (required when hardware/environment available):** minimal CYD smoke with evidence recorded: happy-path `lhs | rhs` (Berry->Berry, `console.stdin()`) and one handled failure/diagnostic case (overflow or unsupported form/consumer), confirming diagnostics/output, exit behavior as testable, and console/stdout restoration. Note device, storage paths, firmware/build details. If unavailable, report concrete blockers and unverified items per `AGENTS.md` (do not claim pass).
- **Optional/if relevant:** targeted integration/QEMU coverage if a deterministic case fits existing harness (not required for this item).

## Expected areas/files (rough)
- `components/shell/src/shell_redirection.{h,c}` — pipeline segment extraction/helpers and temp-capture support as needed
- `components/shell/src/shell_script.c` (and related dispatch headers) — v1-gated pipeline execution, diagnostics, status/restore handling
- `components/berry_lang/src/berry_runtime.cpp` and/or console binding implementation — `console.stdin()` runtime state/API
- `frontend/src/cos-sdk/console.d.ts` — `stdin()` declaration/docs (TSDoc) in sync with binding
- `test/` — host unit tests for shell pipeline/runtime behavior and mocks/stubs as needed
- `docs/reports/` — implementation/HW validation evidence (and update this/umbrella/consolidated plan links on completion)

## Dependencies / prerequisites
- Existing pipeline parse/diagnostic foundation and stdout redirection/fd mechanics in shell components.
- Writable VFS temp location available at runtime for capture file (implementation must handle/diagnose I-O failures cleanly).
- Consistency with R21 command-script execution contract for Berry `rhs` scripts.
- **Coordination / merge gate (2026-11-16 review, updated):** overlaps with other ready work touching Berry runtime/shell/SDK surfaces (`r18-async-await-runtime`, `r22-m4-time-read-only`, `berry-runtime-service-inspection-v1`). No single fixed implementation order is required, but PRs landing close together must rebase/coordinate on shared runtime/shell/resolver changes (including R22 `/flash/scripts` default resolver search if landed) and pass a shared smoke set before merge: shell commands/runtime smoke, Berry runtime lifecycle/teardown (`berry run`/`exec`/`close` and affected command scripts), and `console`/SDK binding sync checks where touched. Record the exact smoke commands/results in each PR.

## Risks / constraints
- ESP32 RAM/flash/temp-storage limits: 8 KiB cap bounds capture, but temp-file creation/write/read/delete and dispatch still require care.
- Global in-process stdout redirection is not concurrency-safe beyond current shell model; restoration/cleanup must be robust on all paths.
- ESP-IDF log output may bypass stdio capture (documented limitation).
- `console.stdin()` is new script-facing API surface: must match Berry name/args/return and SDK typing exactly.
- Temp-file uniqueness/cleanup failures could leave artifacts or affect later runs; implementation must minimize and handle this.

## Progress / evidence
- 2026-11-15: item created from R20 clarification (user prioritized pipes v1; umbrella `../needs-clarification/r20-deferred-backlog.md` retained). Reviewed R20 plan/reports.
- 2026-11-15 (clarification): user decisions recorded — `console.stdin() -> string|nil`; Berry-only `rhs` in v1; only 2-stage `lhs | rhs` (no 3+ stages, no pipe+redirection); 8 KiB hard cap/no truncation; `lhs` any capturable stdout producer (best-effort); temp VFS file capture with cleanup; return `rhs` status when piped execution runs (lhs non-zero with captured output still piped), handled non-zero diagnostics on overflow/wiring/unsupported/capture/setup failures; minimal CYD HW smoke required (happy path + one failure/diagnostic); stdout-only capture caveat accepted/documented. Open questions: none.
- 2026-11-15 (approval): explicitly approved by user in chat and moved to `docs/plan/ready/r20-pipes-runtime-v1.md` (pre-approved for implementation/PR flow; merge remains human). No implementation yet.
- 2026-11-16 (review follow-up): added shared runtime/shell merge-gate coordination with R18/R22/inspection and clarified hardware validation as required when available with blocker reporting.
