# R20 Pipes (`|`) — Parse/Diagnostic Foundation Design

**Date:** 2026-11-10  
**Status:** Design + parse/diagnostic implementation (no runtime pipe execution)  
**Scope:** Recognize unquoted pipe operators (`|`) in shell command lines and provide deterministic diagnostics. Runtime pipeline execution/streaming remains out of scope.

## Goals

- Detect pipe syntax safely (quote-aware) so `|` is not treated as a normal argument.
- Provide clear, non-destructive diagnostics when pipes are used (`pipes are not supported yet`) instead of partial/misleading execution.
- Cover syntax cases with host unit tests; minimize manual hardware validation.

## In scope (this slice)

- Quote-aware detection of unquoted `|` characters (single/double quotes matching existing shell quoting behavior).
- Pipeline segmentation for diagnostics/parsing: split by unquoted `|` into command segments.
- Validation/diagnostics for:
  - any use of pipes (unsupported runtime in this slice) — reported as a handled shell error,
  - empty/missing commands (leading `|`, trailing `|`, consecutive `||` segments),
  - interaction with redirection syntax in piped commands (e.g. `a | b > out`) — still diagnosed as unsupported pipe usage in this slice to avoid ambiguous behavior.
- Quoted `|` characters inside arguments (e.g. `echo "a|b"`) are **not** pipe operators.
- `||` (logical OR) is **not** supported; it is treated as pipe-related syntax and rejected/diagnosed in this slice (conservative, avoids misinterpretation).

## Out of scope (deferred)

- Runtime pipeline execution, stdout/stdin streaming between commands, buffering, backpressure.
- Process/task model for concurrent pipeline stages (commands run in-process today).
- Exit-status semantics for pipelines (`pipefail`, last-status rules).
- Stderr piping, FD duplication (`|&`), command substitution, subshells.
- Input redirection (`<`) and complex redirection combinations in pipelines.

## Proposed parsing model

```c
typedef enum {
    SHELL_PIPELINE_NONE = 0,      // no unquoted pipe operator
    SHELL_PIPELINE_UNSUPPORTED,   // pipe syntax present (recognized) but runtime not supported
    SHELL_PIPELINE_INVALID,       // malformed pipe syntax (empty segment, etc.)
} shell_pipeline_status_t;

typedef struct {
    shell_pipeline_status_t status;
    size_t segment_count;         // number of pipe-separated segments when pipe syntax is present
} shell_pipeline_info_t;

esp_err_t shell_parse_pipeline(const char *line, shell_pipeline_info_t *out);
```

Notes:
- `ESP_OK` with `SHELL_PIPELINE_NONE` means no pipe handling needed.
- `ESP_OK` with `SHELL_PIPELINE_UNSUPPORTED` means the line is a syntactically recognizable pipeline but must be rejected by dispatch with a clear "pipes not supported yet" message (handled, non-zero status).
- `ESP_OK` with `SHELL_PIPELINE_INVALID` (or `ESP_ERR_INVALID_ARG`) means malformed pipe syntax; dispatch reports a syntax error.
- Implementation detail: scan is quote-aware; `||` is rejected as unsupported/invalid for this slice.

## Dispatch behavior (this slice)

- `shell_dispatch_expanded_line()` checks for pipe syntax **before** redirection parsing/execution.
  - `NONE`: continue with existing redirection/command dispatch path.
  - `UNSUPPORTED`: print `pipes: not supported yet` (or equivalent), set non-zero status, return handled.
  - `INVALID`: print `pipes: invalid syntax`, set non-zero status, return handled.
- This prevents commands from being executed with `|` misinterpreted as an argument and avoids ambiguous interaction with `>`/`>>`.

## Examples

| Input | Result |
|---|---|
| `echo hi` | `NONE` |
| `echo "a|b"` | `NONE` |
| `cat a.txt | grep b` | unsupported pipe (diagnosed) |
| `cat a.txt|grep b` | unsupported pipe (diagnosed) |
| `a | b | c` | unsupported pipe, 3 segments |
| `a | b > out.txt` | unsupported pipe (diagnosed; no runtime redirection/pipeline) |
| `| a` | invalid syntax |
| `a |` | invalid syntax |
| `a || b` | invalid/unsupported in this slice |

## Validation

- Host unit tests for parser and dispatch diagnostics: quoted vs unquoted pipes, attached/spaced forms, multiple segments, leading/trailing/consecutive pipes, `||`, and pipe+redirection text.
- Regression: existing shell/redirection/script tests pass.
- **Manual hardware testing minimal:** optional single smoke check (e.g. `echo hi | cat` reports `pipes: not supported yet`, console remains usable). No file I/O or device-specific behavior required.

## Future (deferred)

- Define runtime pipeline semantics (buffered sequential stages vs concurrent streaming), stdin abstraction for native/script commands, exit status, and interaction with redirections per stage.
- Implement minimal runtime support only after semantics and memory/stack constraints are evaluated.