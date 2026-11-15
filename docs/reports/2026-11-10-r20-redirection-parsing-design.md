# R20 Scoped Slice — Shell Redirection Parsing & Semantics Design (Foundation)

**Date:** 2026-11-10  
**Status:** Design + parse-only implementation (no execution wiring)  
**Scope:** `>` and `>>` stdout redirection syntax recognition/parsing for COS shell command lines. Pipes (`|`) and runtime redirection/streaming are **out of scope** for this slice.

## Goal

Provide a small, testable parsing foundation for R20 (pipes/redirection) by:

- Defining initial syntax/semantics for `>` and `>>`.
- Implementing a pure parser/helper that extracts command text and redirection target without performing I/O.
- Preserving current dispatch behavior until explicit runtime wiring is added in a later slice.

## In scope (this slice)

- Recognize unquoted/unescaped redirection operators:
  - `>` — truncate/overwrite output target (semantics defined for future runtime; **not executed now**)
  - `>>` — append output target (future runtime; **not executed now**)
- Common shell-like forms (spaces optional around operator):
  - `cmd arg > file`
  - `cmd arg>file`
  - `cmd arg >> /path/out.txt`
  - `cmd arg>>file`
- Quoted command arguments remain the responsibility of existing argv splitting (`esp_console_split_argv`); quoted target names are supported when the target token is quoted as a whole (e.g. `cmd > "out file.txt"`).
- Deterministic parse errors for unsupported/ambiguous forms in this slice (no guessing).

## Out of scope (deferred)

- **Execution/runtime behavior:** opening files, truncation/append, stdout/stderr capture, stream plumbing, restoring streams on error, file permission/path failure handling at runtime.
- **Pipes:** `|` tokenization/execution.
- **Additional operators:** `<`, `2>`, `2>>`, `&>`, heredocs, command substitution, globbing.
- **Multiple redirections in one command:** e.g. `cmd > a > b` or mixed operators — rejected as parse error in this slice.
- **Redirection before command or in unusual positions:** only a single trailing redirection clause at end-of-command is accepted for now (`<command...> <op> <target>`), to keep semantics predictable.
- **FD-specific behavior:** only standard output redirection intent is modeled.

## Proposed syntax & parse rules

1. **Scanning:** parser scans the raw command line left-to-right while tracking single/double quote state (matching `esp_console_split_argv` quote characters) to find redirection operators that are **not inside quotes**.
2. **Operator detection:** first unquoted `>` determines candidate operator:
   - `>>` if immediately followed by another `>`
   - `>` otherwise
   - `>>>` or longer sequences are invalid (`ESP_ERR_INVALID_ARG` / parse error).
3. **Single redirection only:** if another unquoted `>` operator appears after the target is parsed, reject as unsupported/multiple redirection.
4. **Target:** required non-empty target token after operator (whitespace allowed between operator and target). Target is the next argument token (respecting quotes). Leading/trailing whitespace around the full redirection clause is ignored.
5. **Command portion:** everything before the operator is the command line to dispatch/tokenize; it must contain at least one non-whitespace character (command name).
6. **Quoted targets:** if target is wrapped in matching single or double quotes, parser returns the unquoted inner path string (no escape processing beyond quote stripping, consistent with simple path handling). Unterminated quotes are parse errors.
7. **No target / empty command:** parse error.
8. **Lines without unquoted redirection:** parse result indicates `NONE`; caller should use existing dispatch path unchanged.

## API shape (implemented for this slice)

```c
typedef enum {
    SHELL_REDIRECT_NONE = 0,
    SHELL_REDIRECT_TRUNCATE,  // >
    SHELL_REDIRECT_APPEND,    // >>
} shell_redirect_type_t;

typedef struct {
    shell_redirect_type_t type;
    char command[SHELL_REDIRECT_CMD_MAX]; // command portion (trim-safe copy)
    char target[VFS_PATH_MAX];             // output target path/token
} shell_redirection_t;

// Parse redirection clause from a command line.
// Returns ESP_OK with type==NONE when no redirection is present.
// Returns ESP_OK with type!=NONE when a supported redirection is parsed.
// Returns ESP_ERR_INVALID_ARG for malformed/unsupported redirection syntax.
// Returns ESP_ERR_INVALID_SIZE when buffers/length constraints are exceeded.
esp_err_t shell_parse_redirection(const char *line, shell_redirection_t *out);
```

`SHELL_REDIRECT_CMD_MAX` is chosen to match the existing input line capacity (`INPUT_LINE_MAX`, 128) to avoid expanding command-line limits in this slice.

## Examples

| Input | Result |
|---|---|
| `echo hi` | `NONE` |
| `echo hi > /sdcard/out.txt` | `TRUNCATE`, cmd=`echo hi`, target=`/sdcard/out.txt` |
| `echo hi>>out.txt` | `APPEND`, cmd=`echo hi`, target=`out.txt` |
| `ls > "my file.txt"` | `TRUNCATE`, cmd=`ls`, target=`my file.txt` |
| `> out.txt` | invalid (missing command) |
| `echo hi >` | invalid (missing target) |
| `echo hi > a > b` | invalid (multiple redirections, deferred) |
| `echo hi >>> out.txt` | invalid operator |
| `echo "a > b"` | `NONE` (`>` quoted) |

## Dispatch integration plan (future, not done here)

- `shell_dispatch_expanded_line()` / native command execution will detect parsed redirection, open target via VFS with truncate/append as appropriate, redirect stdout for command duration, and reliably restore stdout and report file errors.
- Script and native command paths must share consistent behavior.
- `stderr` handling and pipes remain follow-up design items.

## Validation

### Parse-only slice (2026-11-10)
- Host unit tests for parser behavior and regressions in existing shell dispatch/tokenization assumptions (where applicable).
- No hardware validation required for parse-only logic.

### Runtime wiring slice (2026-11-10)
- `shell_dispatch_expanded_line()` now parses redirection and, for supported `>`/`>>` cases, redirects stdout around command dispatch via `shell_redirect_begin/end` (VFS path resolution/open + stdout redirect/restore; fd-level redirect with stdio FILE* swap fallback for ESP-IDF console compatibility). Parse/redirect errors are reported to the console with non-zero status.
- Host regression tests rebuilt/passed: `test_shell_script` 23/23, `test_shell` 63/63, `test_shell_berry` 28/28 (no redirection execution cases exercised in host tests; stdout redirection behavior is environment-dependent).
- **Hardware validation (2026-11-10, user):** ✅ basic `>`/`>>` passed on `/sdcard` (`info > ./info.log`, `info >> ./info.log`, file creation/size/content, append, console restored). Error/edge cases and additional forms remain good follow-up checks. Guide/results: `docs/reports/2026-11-10-r20-redirection-runtime-hw-guide.md`.
