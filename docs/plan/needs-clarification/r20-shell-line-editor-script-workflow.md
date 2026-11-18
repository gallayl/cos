# R20c: Shell line-editor seam + on-device script workflow polish

- **ID:** r20-shell-line-editor-script-workflow
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item c: shell line-editor refactor seam); docs/plan/done/r20-pipes-runtime-v1.md; docs/plan/2026-11-12-berry-command-script-migrations.md (R22); docs/plan/consolidated-improvement-plan.md (R20)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Clarify and deliver a small, practical improvement around the deferred shell line-editor refactor seam and day-to-day Berry command/script workflow on device. Goal: reduce friction when running/debugging scripts (clear resolver/path feedback) with a minimal, low-risk seam refactor in the shell input layer.

## Scope (decisions recorded 2026-11-18)
- **Line-editor refactor seam (v1, minimal):** extract key/escape parsing into a small helper/module (e.g. `shell_keyparse.*`) with unit-testable pure-ish functions; wire into `components/shell/src/shell_input.c/h` with **no UX change**. This is the only seam work in scope for this item unless explicitly expanded later.
- **Script-workflow affordances in v1 (CLI, on-device):** add a **`which <name-or-path>`** (or equivalent `type`-style command) that shows the resolved script path using the **same resolver rules** as command execution (no exec). Also improve **resolver/not-found error messages** to mention searched locations (**CWD, listed `path` dirs, `/flash/scripts`**) when helpful, while preserving current “handled vs unknown command” dispatch semantics (do not turn handled resolver errors into `Unknown command`).
- **Out of additional v1 affordances:** no `scripts/list-scripts` command, no stock copy/show helper beyond what `which`/messages provide, no broad `path` command redesign in this item (other than message/help text only if required).

## Non-goals
- Full on-device graphical/text editor UI.
- Redesigning the entire shell input stack beyond the scoped key/escape parser extraction.
- `scripts/list-scripts`, stock script copy/show helpers, or general `path` subcommand redesign (deferred unless separately approved).
- Unscoped “do all of R20” work.

## Acceptance criteria
- [ ] Key/escape parsing extracted into a helper/module with unit-testable behavior; `shell_input.c` wiring preserved, **no user-visible input/UX behavior change** (regression-tested by existing behavior expectations).
- [ ] `which <name-or-path>` (or agreed equivalent name) implemented: resolves via same rules as `shell_script` resolver (CWD → `path` dirs → `/flash/scripts`, explicit paths with/without `.be` behavior consistent with current resolution), prints resolved virtual path or a clear “not found” result; does **not** execute the script.
- [ ] Resolver/not-found messaging improved where in scope to list searched locations (CWD, `path` list, `/flash/scripts`) **without** breaking existing contract (e.g. explicit missing script still reported as handled/script-not-found style messages per current design; do not introduce spurious `Unknown command` for handled cases).
- [ ] Automated checks per `AGENTS.md` for affected scope pass (format-check, tests; build/lint/docs-check as required by touched areas). Host/unit tests added/updated for key parser extraction and for `which`/resolver message behavior where feasible.
- [ ] Minimal manual HW validation (serial console on CYD) when behavior changes: verify `path list/add/remove/clear`, run scripts from CWD/path dir/`/flash/scripts`, verify `which` and improved messages; quick BT keyboard sanity only if input parsing code paths were materially touched in a way that risks regression (document if done/skipped).

## Validation plan
- **Automated:** `make format-check`, `make test`. Add/update host tests (e.g. `test/test_shell_script.c` and/or new unit tests for key parser helper). Run `make build`, `make lint`, `make docs-check` when ESP-IDF/clang-tidy/Doxygen scope requires (per `AGENTS.md`).
- **Hardware/manual (minimal, CYD, serial):** confirm `path` commands, script execution from CWD / configured `path` dir / `/flash/scripts`, `which` resolution for representative cases (bare name, explicit path, missing), and error messages list searched locations where applicable. If input parsing refactor touches editing paths, do a short BT keyboard sanity (arrows/backspace/history) and record result; otherwise note not required. Record blockers if hardware unavailable.

## Expected areas/files (rough)
- `components/shell/src/shell_input.c`, `components/shell/src/shell_input.h` (and new `shell_keyparse.*` if introduced)
- `components/shell/src/shell_script.c/.h`, `components/shell/src/shell_cmds.h` or command registration points for `which` (and any shared resolver helpers)
- `test/test_shell_script.c` and/or new host tests for key parser
- Docs/help text updates only if required by new command/messages (keep minimal)

## Dependencies / prerequisites
- R20 umbrella: `docs/plan/needs-clarification/r20-deferred-backlog.md` (item c context).
- R9/R22 resolver behavior and `path` command semantics (`docs/plan/2026-11-12-berry-command-script-migrations.md`, `components/shell/src/shell_script.c`).

## Risks / constraints
- ESP32 memory/stack and shell input complexity; key-parse extraction must not change timing/behavior.
- Must preserve resolver contract (order, `.be` handling, explicit vs non-explicit, handled-error semantics). Error string changes only where approved in this item (improved messages mentioning searched locations) and must not produce misleading `Unknown command` for handled cases.
- No new dependencies required by design; avoid scope creep into listing/copy helpers.

## Open questions
- None (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A). If implementation reveals a concrete blocker, document it here and do not move to Ready.

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 1).
- 2026-11-18 (clarify-feature): reviewed `components/shell/src/shell_input.c/.h`, `shell_script.c/.h`, R20 backlog and R22 plan context. User choices: 1A (extract key/escape parser helper, no UX change), 2A (`which` + improved resolver messages listing searched locations when helpful), 3A (preserve resolver behavior/contract; error-string improvements allowed only as specified, no misleading `Unknown command` for handled cases), 4A (minimal CYD serial HW matrix; BT sanity only if input parsing touched materially). Open questions resolved; awaiting explicit user approval to mark Ready and move to `docs/plan/ready/r20-shell-line-editor-script-workflow.md`.
