# R20 Runtime Design Spike — Pipes (`|`) and Stderr/FD Redirection

**Date:** 2026-11-10  
**Status:** Design spike (no implementation)  
**Scope:** Evaluate feasible runtime semantics for in-process pipelines and stderr/FD redirection in COS shell, building on existing parse/diagnostic foundations.

## Context / current architecture

- Shell dispatch is in-process (`components/shell/src/shell_script.c`): `shell_dispatch_expanded_line()` performs parse/diagnostics, optional stdout redirection, then executes via `esp_console_run()` (native commands) or script execution (`berry_runtime_run_file` / command-script path) on `ESP_ERR_NOT_FOUND`.
- Command output is primarily emitted with `printf`/`stdout`; ESP-IDF logging (`ESP_LOGx`) may be routed to UART/stdout depending on configuration (not guaranteed to be `stderr` in all builds).
- Stdout redirection (`>`/`>>`) is already implemented (`components/shell/src/shell_redirection.c`): resolve VFS target, `fopen`, `fflush(stdout)`, redirect via fd `dup`/`dup2` when `fileno(stdout)` is usable, otherwise swap `stdout` `FILE*` (ESP-IDF fallback). Restore on end.
- Parse/diagnostic foundations now exist (runtime deferred): `shell_parse_pipeline()` and `shell_parse_fd_redirection()` reject recognizable forms early with handled non-zero diagnostics.

## 1) Runtime pipes (`|`) — feasibility and options

### Constraints
- No process isolation / no POSIX pipes between separate processes; commands run in the same task/context (esp_console handlers and Berry runtime are in-process).
- Memory is limited (embedded target); unbounded buffering is unsafe.
- Stages may be native console commands or Berry scripts/commands; not all commands are designed as stdin consumers.
- Concurrent streaming between in-process stages would require task/queue plumbing and thread-safe stdout capture/routing; significant complexity.

### Option A — Buffered sequential pipeline (recommended first increment)
**Model:** For `A | B | ... | N`, run stages left-to-right sequentially, capturing stdout of stage *i* into a bounded buffer/temp file and providing it as input to stage *i+1*.

- **Capture mechanism:** reuse/extend redirection infrastructure to target an in-memory ring/buffer or temporary VFS file (e.g. `/tmp`/RAM-backed path if available, else `/flash`/`/sdcard` temp with cleanup). Buffer must have a hard cap (configurable, e.g. 4–16 KB) and defined overflow behavior (fail pipeline with diagnostic, or truncate with warning).
- **Input delivery:** requires a way for stage *i+1* to read prior output:
  - *Native commands:* only feasible if command supports an explicit input argument/path/stdin convention; many existing commands likely do not. A generic stdin abstraction for `esp_console` handlers does not exist.
  - *Berry scripts:* could pass captured text as an argument (e.g. `args` convention) or provide a runtime helper/API to read piped stdin; needs contract decision.
- **Pros:** simpler than concurrency, deterministic, easier resource control.
- **Cons:** no true streaming/backpressure; large intermediate output hits limits; limited compatibility unless commands/scripts adopt stdin conventions.
- **Exit status:** common shell choice is last-stage status; `pipefail`-like behavior out of scope initially.

### Option B — Concurrent streaming with tasks/queues
**Model:** run stages in separate tasks connected by bounded queues/ringbuffers, redirecting each stage stdout into the queue writer.

- **Pros:** enables streaming semantics and avoids storing full intermediate output (only queue capacity).
- **Cons:** high complexity/risk on embedded (task stacks, scheduling, stdout is global/process-like in this context, Berry VM/thread-safety, console handler reentrancy), harder cleanup on failure/timeout.
- **Assessment:** likely too risky/expensive for first runtime implementation; defer unless strong need and proven stdout isolation strategy.

### Option C — Keep pipes parse/diagnostic-only (status quo)
- **Pros:** safe, no semantic surprises.
- **Cons:** no functionality; acceptable until stdin/command compatibility model is defined.

### Recommendation (pipes)
1. **Do not implement runtime pipes yet** without defining a command/script input contract. Current command ecosystem is output-oriented; `A | B` is only meaningful if `B` can consume stdin/buffer input.
2. If pursued, prototype **Option A** behind a narrow scope:
   - Support a small, explicit set of consumers (or a Berry-side stdin API, e.g. `io.stdin()`/runtime-provided buffer) rather than claiming generic pipe support.
   - Buffer to memory with strict size cap (and optional temp-file fallback only if VFS/temp semantics are solid).
   - Sequential execution, last-stage exit status, clear diagnostics on overflow/unsupported consumer.
3. Add unit tests for parser→executor planning and buffer/overflow/error propagation; hardware smoke only for end-to-end happy path once implemented.

## 2) Stderr/FD redirection runtime — feasibility and options

### Constraints
- COS output sources are mixed: `printf`/`stdout` (capturable by current stdout redirection), and `ESP_LOGx` which typically formats to UART/console; routing can depend on `esp_log_set_vprint`, log timestamp/color config, and target console channel. Treating logs as `stderr` fd is not portable/guaranteed.
- Current redirection implementation manipulates `stdout` (fd 1 / `FILE* stdout`). Extending to fd 2 requires `stderr`/fd 2 to be meaningful and consistently used by producers.
- `&>` / `&>>` imply capturing both stdout and stderr into one target; semantics depend on ordering/interleaving guarantees (usually unspecified).

### Option A — Stdout-only redirection remains canonical; stderr/FD stays diagnostic (safe baseline)
- Keep `2>`, `2>>`, `&>`, `&>>` as recognized unsupported diagnostics until runtime guarantees are defined.
- Document that `>`/`>>` captures stdout/printf output; ESP logs may or may not be captured depending on platform routing.
- **Pros:** no false guarantees.
- **Cons:** cannot redirect error/log streams explicitly.

### Option B — Best-effort fd-based stderr redirection (`2>`/`2>>`) + combined (`&>`)
**Model:** mirror stdout redirection for `stderr` (fd 2 / `FILE* stderr`) when `fileno(stderr)` is usable; for `&>`, redirect both stdout and stderr to the same target.

- **Implementation sketch:** extend `shell_redirect_state_t` and begin/end to manage multiple streams; open target once, duplicate fd for both streams or `dup2` same target fd; restore both.
- **Caveat:** only captures writes that actually go to `stderr`/fd 2. `ESP_LOGx` output may bypass this if it writes directly to UART/vprintf hook not tied to stderr fd.
- **Pros:** familiar shell syntax if producers use stderr.
- **Cons:** potentially misleading on ESP-IDF unless log sink is aligned; more state/restore complexity.

### Option C — Log-capture aware redirection (advanced)
**Model:** temporarily install a custom `esp_log_set_vprint` handler and/or route log output to redirected stream during command execution.

- **Pros:** can capture ESP logs in addition to stdio, closer to user expectation for `2>`/`&>` in embedded contexts.
- **Cons:** global process-wide effect (concurrency/reentrancy issues), must preserve existing log formatting/locking behavior, risk of interfering with console transport; needs careful nesting/restore and measurement.
- **Assessment:** feasible as an opt-in/experimental mode but requires careful design and likely separate milestone.

### Recommendation (stderr/FD)
1. **Short term:** keep runtime as diagnostic-only (current slice) and explicitly document scope: `>`/`>>` redirects stdout/printf; `2>`/`&>` semantics require runtime contract decision.
2. **If implementing runtime next:** start with **Option B (fd/FILE* stderr best-effort)** for `2>`/`2>>` and `&>`/`&>>` with clear docs/tests, because it composes with existing redirection mechanics. Treat ESP log capture as a separate follow-up (Option C) only if needed and after validating log routing behavior on target.
3. **Safety requirements:** redirect/restore must be exception-safe across command/script failures; never leave console stdout/stderr permanently redirected; unit-test state transitions/misuse; minimal HW smoke for `2>`/`&>` only after runtime implementation.

## 3) Suggested implementation order (after this spike)

1. **Stderr/FD runtime (best-effort, limited scope)** — smaller delta from existing stdout redirection (`shell_redirect_*` extension), clearer semantics if documented as stdio-fd capture. Add runtime support for `2>`/`2>>` first, then `&>`/`&>>`; keep diagnostics for unsupported/ambiguous combinations.
2. **Pipes runtime prototype (very limited)** — only after defining stdin contract for at least one consumer path (e.g. Berry script helper or explicit command support). Start with single pipe + bounded in-memory capture + sequential execution.
3. **Advanced log capture / streaming pipelines** — only with demonstrated need and target measurements.

## 4) Test strategy implications

- **Unit-testable now/next:** parser/planner edge cases, redirection state machine, buffer cap/overflow handling, restore-on-error paths (host tests/mocks), dispatch ordering (pipes/stderr/stdout precedence).
- **Hardware-minimal:** end-to-end smoke for actual fd routing/log behavior on ESP-IDF console, since host stdio/log routing differs from device.

## 5) Plan impact

- R20 now has parse/diagnostic foundations for `>`/`>>` (runtime done), `|` (diagnostic), and `2>`/`2>>`/`&>`/`&>>` (diagnostic). This spike records feasibility and recommends **stderr/FD best-effort runtime** as the more tractable next implementation candidate, with **pipes runtime deferred pending stdin/consumer contract**.
