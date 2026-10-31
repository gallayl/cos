# COS improvement implementation plan

This document is the prioritized implementation plan and progress tracker for
the COS shell, display console, and Berry runtime. Checkboxes track completion;
effort, ROI, and risk are relative estimates.

**Effort:** S = focused change; M = several modules or meaningful edge cases;
L = new behavior with architectural, resource, or integration concerns.
**ROI:** expected value relative to implementation effort.
**Risk:** likelihood and impact of regressions or difficult-to-diagnose behavior.

## Milestone 1 — shell usability and CI correctness

**Status:** Complete

### Scope and acceptance criteria

- `cat <path>` outputs the entire file, not just its first 511 bytes, using
  bounded memory and preserving the existing newline behavior.
- File-open/read failures and output failures are reported; directories remain
  rejected as before.
- The shell registers ESP-IDF's `help` command, and `help` lists registered COS
  commands.
- Integration-test artifact uploads include every firmware target built by
  `make integration-test-build`, including `test_nvs_calibration`.
- Regression tests cover long `cat` output and command discovery.
- Relevant tests, firmware build, formatting, and static analysis pass.

### Progress

- [x] Add a bounded VFS streaming-read API.
- [x] Convert `cat` to stream all content without a whole-file allocation.
- [x] Register the help command and cover its output.
- [x] Add integration coverage for a file larger than the previous limit.
- [x] Align integration firmware and log artifact lists with built targets.
- [x] Build firmware locally with ESP-IDF 6.1.
- [x] Run `make format-check` locally; CI Format Check also passes.
- [x] Add host mock declarations/stubs for the new help and VFS stream APIs.
- [x] Pass host unit tests after the mock fixes (CI run `41186872616`).
- [x] Pass CI firmware build and clang-tidy (run `41186872518`).
- [x] Pass QEMU shell integration tests (CI run `41186872503`).
- [x] Record final validation results and mark milestone complete.

## Prioritized backlog

| Priority | Improvement | Effort | ROI | Risk | Dependencies / notes |
|----------|-------------|--------|-----|------|----------------------|
| 1 | Stream large files with `cat` | S–M | High | Low | Implemented in Milestone 1; VFS streaming also supports future file-copy work. |
| 2 | Make `help` and command discovery reliable | S | High | Low | Implemented in Milestone 1; keep help metadata and README in sync as commands are added. |
| 3 | Repair integration-test artifact coverage | S | High | Low | Implemented in Milestone 1; artifact inventories should follow the Makefile build list. |
| 4 | Complete everyday file operations | M | High | Medium | Add `cp`, `mv`, and explicit `rmdir`; define overwrite, cross-filesystem, and failure cleanup semantics. Add recursive directory copy only with explicit scope/option. Use bounded streaming. |
| 5 | Add keyboard routing for shell and Berry apps | M–L | High | High | Bluetooth keyboard currently feeds shell input; Berry GUI uses touch polling/timers. Requires shared input ownership, typed events, app lifecycle behavior, and mode-transition tests. |
| 6 | Add interactive line input for Berry scripts | M–L | High | High | Support prompt/read-line/Enter workflows without blocking render/timer or shell tasks. Define asynchronous completion, echo, cancellation, and prompt ownership. Depends on shared keyboard routing. |
| 7 | Expose structured COS services to Berry | M–L | High | Medium–High | Provide APIs such as `fs.list(path)` and `net.ping(host)` that return structured values/errors. Share the underlying service with shell commands. `action(command)` can remain a compatibility bridge, but avoid parsing command output. |
| 8 | Add command and path autocomplete | M | High | Medium | Tab currently enters a tab character; command history and line editing already exist. Complete command names first, then paths; preserve the current line and handle ambiguity and buffer limits. |
| 9 | Add display-console scrollback and touch scrolling | M–L | Medium–High | High | Current text buffer discards the top row on scroll. Requires bounded history, viewport rendering, gesture handling, and touch ownership arbitration with Berry UI. |
| 10 | Add network diagnostics and downloads | M–L | Medium–High | High | Start with `ping`/`nslookup`, then a bounded HTTP(S) download. Define timeouts, TLS validation, memory/storage limits, and cleanup. Share network service with Berry API. |
| 11 | Improve shell/editor ergonomics | S–M | Medium | Low–Medium | Add a direct `clear` command; assess 128-character input and eight-entry history limits against memory and keyboard behavior. |
| 12 | Refactor shell input for testability | M | Medium | Medium | UART transport, parser/editor/history, and command execution currently live in one module; its unit test includes the implementation directly. Extract a focused editor seam to enable keyboard routing and Berry input. |
| 13 | Clarify and reduce CI maintenance | S–M | Medium | Low–Medium | Align/pin tool versions where useful; reduce repeated setup only where workflows share an environment. Add caching/cancellation only when it measurably improves CI. |

## Milestone 2 — file operations and command completion

**Status:** Planned

- [ ] Define `cp`, `mv`, and `rmdir` syntax, aliases, overwrite policy, and
  same- versus cross-filesystem behavior.
- [ ] Implement bounded file copying and handle short reads/writes, full media,
  and destination cleanup on failure.
- [ ] Implement move semantics with a same-filesystem rename when safe and a
  documented copy/remove fallback when needed.
- [ ] Decide whether recursive copy is in scope; if included, guard against
  copying a directory into itself and bound traversal resources.
- [ ] Add command completion, then VFS path completion, without losing the
  current edit buffer or history behavior.
- [ ] Add unit/integration coverage for success, invalid paths, overwrite,
  capacity/error conditions, and completion ambiguity.

## Milestone 3 — keyboard and Berry interaction/API

**Status:** Planned

### Input contract

- [ ] Define a shared key-event representation (text/key identity and, if
  supported by the input device, press/release and repeat).
- [ ] Route input to exactly one active owner: shell in console mode or focused
  Berry app in GUI mode.
- [ ] Ensure app close/error restores shell ownership and cannot leave keys
  stuck or a hidden shell line modified.
- [ ] Keep callbacks/task work bounded; protect VM calls and app teardown using
  the existing Berry runtime/UI synchronization patterns.

### Berry features

- [ ] Add an opt-in keyboard control mapping to Breakout while preserving touch
  play; document controls in the example.
- [ ] Define a non-blocking console script API for prompt/read-line/Enter,
  including callback/result delivery, echo, cancellation, and app ownership.
- [ ] Add a structured read-only `fs.list(path)` API for Berry apps.
- [ ] Add a shared ping/DNS service and structured `net.ping(host)` result only
  after timeouts, no-network behavior, and resource limits are specified.
- [ ] Keep `action(command)` backward-compatible and document that its return
  value is status/error, not structured command output.
- [ ] Test input routing across console/GUI transitions, script cancellation,
  malformed paths, empty directories, and network error/timeout paths.

## Milestone 4 — console scrollback and network utilities

**Status:** Planned

- [ ] Add a bounded scrollback ring separate from the visible text grid.
- [ ] Define viewport behavior while new output arrives and when the user
  returns to the bottom.
- [ ] Add touch scrolling only while the text console owns the display; preserve
  Berry UI touch dispatch.
- [ ] Add ping and DNS lookup shell commands using shared, time-bounded services.
- [ ] Specify HTTP(S) download size limits, TLS validation, cancellation,
  destination handling, and partial-file cleanup before implementing.
- [ ] Test scrollback/viewport independent of physical gestures, then validate
  gestures and networking on supported hardware.

## Milestone 5 — usability and maintenance follow-up

**Status:** Planned

- [ ] Add/document a concise `clear` command.
- [ ] Review shell line/history limits and their static RAM cost before changing
  defaults.
- [ ] Extract the shell line editor behind a testable interface when needed by
  keyboard routing; retain behavior with focused unit tests.
- [ ] Review CI tool version pinning, repeated package setup, artifact
  inventories, and failure log retention.
- [ ] Keep README command/API documentation current with shipped behavior.

## Berry API risk, complexity, and ROI

Ratings are relative. Risk includes integration hazards as well as code size;
ROI considers reuse across GUI apps, console scripts, and shell commands.

| Capability | Complexity | Risk | ROI | Main risks and controls |
|------------|------------|------|-----|-------------------------|
| Keyboard routing in console and GUI modes | Medium–High | High | High | Events may reach both shell and app, keys can get stuck, or callbacks can race teardown. Centralize ownership; test repeat and mode transitions. |
| Asynchronous line input for Berry scripts | Medium–High | High | High | Blocking input can stall rendering/timers or monopolize shell processing. Use one input owner, asynchronous completion, and explicit cancellation/echo semantics. |
| `fs.list(path)` and read-only filesystem APIs | Medium | Medium | High | Mount changes, result bounds, path normalization, and Berry heap pressure. Return bounded structured entries and explicit errors; reuse VFS checks. |
| `net.ping(host)` / name resolution | Medium–High | Medium–High | Medium–High | Network calls can block, time out, or consume task stack. Define bounded/asynchronous behavior, timeout, result shape, and no-network errors; share service with shell. |
| `action(command)` compatibility bridge | Low | Medium | Medium | Shell output is not structured and scripts couple to command strings. Retain for compatibility, but avoid building GUI behavior around output parsing. |

## Milestone 1 validation log

| Validation | Status | Result |
|------------|--------|--------|
| Host unit tests | Pass | Branch CI run `41186872616` passed after adding both missing mock functions. |
| QEMU shell integration tests | Pass | Branch CI run `41186872503` passed, including long-file `cat` and `help` regression tests. |
| ESP-IDF firmware build | Pass | Local `espIdfCommands build` passed with ESP-IDF 6.1; CI build passed in run `41186872518`. |
| clang-format check | Pass | `make format-check` passed locally; branch CI Format Check passed in run `41186872517`. |
| clang-tidy | Pass | CI clang-tidy passed in run `41186872518`. Local `make lint` is not representative with clang-tidy 21 and ESP-IDF 6.1 compile flags. |
| Doxygen check | Pass | Branch CI Doxygen Check passed in run `41186872639`. |
