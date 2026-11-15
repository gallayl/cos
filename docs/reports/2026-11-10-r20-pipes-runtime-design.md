# R20 Pipes (`|`) — Runtime Design (Minimal Prototype Proposal)

**Date:** 2026-11-10  
**Status:** Design proposal (no implementation)  
**Prereqs:** `shell_parse_pipeline()` / dispatch diagnostics in place (`docs/reports/2026-11-10-r20-pipes-parsing-design.md`), stdout redirection runtime available for capture patterns (`components/shell/src/shell_redirection.{h,c}`), runtime feasibility spike `docs/reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`.

## Goal

Define a minimal, safe, in-process pipeline runtime for COS shell that can be implemented incrementally and validated mostly with unit tests (manual hardware smoke minimal).

## Constraints (current architecture)

- **In-process execution:** stages run via `esp_console_run()` (native handlers) or Berry script/command runtime (`berry_runtime_run_file` / command execution) in the same task/context. No POSIX process pipes.
- **No generic stdin stream:** esp_console handlers receive `argc/argv`; Berry command apps receive `run(args)` list (R21). There is no standard stdin/fd-0 consumption API for arbitrary commands.
- **Output capture:** stdout can be redirected to file/fd/stdio stream (existing redirection). ESP-IDF log output may bypass stdio fd capture depending on routing.
- **Embedded limits:** memory/stack constrained; intermediate pipeline data must be bounded.

## Proposed minimal runtime scope (v1)

### Supported form
- Single pipe: `lhs | rhs` (exactly 2 stages), where `lhs` produces output and `rhs` is a **pipeline-aware consumer** (explicit opt-in contract below).
- Multi-stage pipelines (`a | b | c`) and pipe+redirection combinations remain **unsupported/diagnosed** in v1 unless explicitly implemented after v1 validation (parser already reports segment count; executor can gate on `segment_count == 2`).

### Execution model (sequential buffered)
1. Parse/validate pipeline (existing). If unsupported for v1 scope, keep diagnostic behavior.
2. Execute `lhs` with stdout captured to a **bounded buffer** (preferred, in-memory) or a temporary VFS file fallback only if needed.
   - Buffer cap: fixed/configurable small limit (proposal: **8 KiB** default, hard fail on overflow with clear diagnostic; no silent truncation in v1).
   - Capture should reuse stdio/fd redirection mechanics where possible (e.g. redirect stdout to temp file/pipe-like sink abstraction) but keep implementation simple and exception-safe (restore stdout on all paths).
3. If `lhs` dispatch fails in an infrastructure sense, stop and report error. Exit-status propagation policy (v1): return **`rhs` status** on successful pipeline wiring/execution path, but report capture/overflow/wiring failures as handled non-zero errors (documented; `pipefail`-like behavior deferred).
4. Execute `rhs` with captured text supplied via the **stdin contract** (below).
5. Cleanup temp resources.

### Stdin/consumer contract (key decision)

Because existing commands are not generic stdin consumers, v1 must define an explicit contract:

**Option 1 (recommended for v1, Berry-first): piped input as runtime-provided data**
- Add a Berry-accessible mechanism for command scripts, e.g.:
  - global/module value set only for piped execution: `shell.stdin()` / `input.read()` returning string or `nil`, or
  - `_cos_pipe_input` global string (cleared/restored after execution), or
  - pass captured text as an extra argument convention (e.g. append to `run(args)` only when piped) — **requires explicit contract to avoid breaking existing scripts**.
- **Pros:** enables useful script consumers (`grep`-like Berry scripts, transforms) without changing native esp_console ABI.
- **Cons:** native C commands cannot consume it without per-command changes.

**Option 2: argv injection for consumers that opt in**
- If `rhs` is invoked as `cmd [args...]`, supply piped data as final arg or via a sentinel (e.g. `cmd --stdin <data>` / temp file path). Not suitable for large/binary-ish data and shell-quoting sensitive.
- **Pros:** no runtime global state.
- **Cons:** awkward/unsafe for arbitrary text, size-limited by argv/tokenization constraints.

**Option 3: temporary file path contract**
- Capture to temp file and invoke `rhs` with an implicit/explicit input path convention. Requires writable temp location and cleanup guarantees.
- **Pros:** handles larger data than argv; file-based consumers easy.
- **Cons:** path management, cleanup on failure, storage wear/availability.

**Recommendation:** start with **Option 1 (Berry-first)** using a clearly-scoped runtime API/global for piped stdin (string, bounded), and restrict v1 runtime execution to pipelines whose `rhs` is a **Berry script/command** that can consume it (or a very small allowlist). Native-to-native piping remains deferred until a native stdin abstraction exists.

## Dispatch/executor integration sketch

- In `shell_dispatch_expanded_line()`:
  - after `SHELL_PIPELINE_UNSUPPORTED` detection, if v1 runtime is enabled and pipeline matches v1 shape (2 segments, supported consumer form), route to `shell_execute_pipeline(...)` instead of diagnostic.
  - otherwise keep current diagnostics.
- New helpers (proposed): `shell_parse_pipeline_segments()` (fill segment command strings), `shell_execute_pipeline(line/info, ret)` (capture + run stages + restore).
- Capture implementation options:
  - redirect stdout to a temp VFS file, run `lhs`, read file back into bounded buffer (simple, reuses existing file redirection path), delete temp.
  - or in-memory stdio capture if a portable memstream/fd-backed buffer is feasible on ESP-IDF/newlib (verify target support before relying on it).

## Error handling / diagnostics

- Overflow: `pipes: input too large (max N bytes)` (handled, non-zero).
- Unsupported v1 shape/consumer: keep/return clear diagnostic (e.g. `pipes: not supported yet` or `pipes: unsupported pipeline form`).
- Capture/setup failure: report and restore streams/global state.
- Always restore stdout and clear piped-input state, even on command/script errors.

## Test strategy (minimal HW)

- **Host unit tests (primary):** segment parsing/planning, v1 gating (2-segment only), bounded-buffer overflow behavior, stdin contract delivery/cleanup, stdout restoration on failure paths, exit-status mapping. Use mocks/temp host files where appropriate.
- **Manual hardware smoke (minimal, after implementation):** one happy-path Berry-to-Berry or producer-to-script case writing/reading a small payload; confirm console restored and no memory issues. Defer broad matrix to unit tests.

## Open questions (need confirmation before implementation)

1. **Stdin contract form:** runtime helper/global (and exact name/API) vs argv injection vs temp-file convention.
2. **v1 consumer scope:** Berry scripts only, or allow specific native commands?
3. **Multi-stage/combined forms:** keep 3+ stages and pipe+redirection unsupported in v1?
4. **Exit status:** last stage status vs fail-fast/pipefail-like behavior.

## Recommended implementation steps

1. Confirm stdin contract + v1 scope (this doc).
2. Implement segment extraction and v1-gated executor with bounded temp-file capture (portable) and Berry stdin delivery.
3. Add host unit tests; run existing shell suites.
4. Minimal HW smoke; update plan/docs. Keep runtime behind conservative diagnostics for unsupported forms.
