# R20 — Pipes runtime v1 (minimal sequential buffered pipeline)

- **ID:** r20-pipes-runtime-v1
- **Status:** needs-clarification (specification complete; **explicit approval pending** to mark Ready and move)
- **Created:** 2026-11-15
- **Updated:** 2026-11-15 (clarification completed; approval pending)
- **Source/links:** [`r20-deferred-backlog.md`](./r20-deferred-backlog.md) (umbrella), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20, P3, in progress), [`reports/2026-11-10-r20-pipes-parsing-design.md`](../../reports/2026-11-10-r20-pipes-parsing-design.md), [`reports/2026-11-10-r20-pipes-runtime-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md) (primary proposal), [`reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md)
- **Approval:** pending — ready/pre-approved for implementation/PR flow requires explicit user approval after this specification (merge remains human).

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
- [ ] **Explicit approval to mark Ready/pre-approved for implementation:** pending after specification.

## Open questions
- none (all Ready-contract fields specified below; pending explicit approval only)

## Acceptance criteria / definition of done (draft, to finalize)
- [ ] v1 contract and scope explicitly specified and approved (no open questions).
- [ ] `lhs | rhs` (2-stage) works for approved consumer/producer forms using bounded sequential capture and documented stdin contract.
- [ ] Overflow, unsupported forms, and setup/capture/wiring failures produce clear handled diagnostics/non-zero status; stdout and piped-input state restored on all paths.
- [ ] Host unit tests cover segment planning/gating, capture/overflow, stdin delivery/cleanup, restoration-on-error, and exit-status mapping; existing shell tests remain passing.
- [ ] Docs updated (behavior, limits, unsupported forms, examples as applicable).
- [ ] Validation evidence recorded: `make format-check`, `make test`, firmware build/lint and other scope checks per `AGENTS.md` as applicable; hardware smoke if required by finalized plan.

## Validation plan (draft, to finalize)
- **Automated:** `make format-check`, `make test`. Scope checks per `AGENTS.md`: firmware build (`make build`) and `make lint` if feasible for `components/shell`/runtime changes; integration/QEMU if relevant test coverage added; docs-check if docs/comments affected.
- **Hardware/manual (likely):** minimal smoke on CYD hardware for one happy-path piped case and console restoration/overflow or unsupported-form sanity as specified (pending confirmation).
- **Not in scope unless specified:** broad HW matrix, ESP log capture semantics, multi-stage/streaming performance benchmarking.

## Expected areas/files (rough)
- `components/shell/src/shell_redirection.{h,c}` (pipeline parsing/segment extraction helpers as needed)
- `components/shell/src/shell_script.c` / dispatch + pipeline executor
- `components/berry_lang/` (Berry stdin API/runtime hook if Option 1 chosen)
- `test/` (host shell/script/pipeline unit tests, mocks as needed)
- `integration_test/` (only if feasible/relevant)
- `docs/reports/` (implementation/HW evidence), `README.md` or shell docs if user-facing behavior documented

## Dependencies / prerequisites
- Existing pipeline parse/diagnostic foundation and stdout redirection mechanics (present).
- Decision on Berry stdin API shape and where it lives (shell/berry runtime binding) consistent with R21 command-script contract.

## Risks / constraints
- ESP32 memory limits (capture cap, temp-file availability/wear if file-backed).
- In-process global stdout capture is not concurrency-safe beyond current shell model; must restore on all paths.
- ESP-IDF log output may bypass stdio capture (piped content may exclude logs).
- Contract choice can affect script compatibility; must be explicit and tested.
- Temp-file cleanup on failures/interruptions must be robust.

## Progress / evidence
- 2026-11-15: item created from R20 clarification (user prioritized pipes v1; umbrella retained). Drafted scope/non-goals/AC/validation from `docs/reports/2026-11-10-r20-pipes-runtime-design.md`. Open questions pending user answers. No implementation/approval yet.
