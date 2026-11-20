# R20c: Shell line-editor seam + on-device script workflow polish

- **ID:** r20-shell-line-editor-script-workflow
- **Status:** done (delivered, PR/CI green; pending human review/approval, merge, and hardware/manual validation)
- **Created:** 2026-11-18
- **Completed:** 2026-11-20
- **PR:** [#109](https://github.com/gallayl/cos/pull/109) (head `f1b15a5dc9681c284eab6dceefebfea103307c9a`, branch `feat/r20c-shell-script-workflow`, base `main@54c61c07`)
- **CI:** [44759321767](https://github.com/gallayl/cos/actions/runs/44759321767) — **success** (all required checks green, including `lint / ESP-IDF Build & Lint`, `test / Unit Tests`, and integration tests incl. `Integration Tests (shell_fs)` pass)
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item c: shell line-editor refactor seam); [`../done/r20-pipes-runtime-v1.md`](../done/r20-pipes-runtime-v1.md); [`../2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (R22); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

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
- [x] Key/escape parsing extracted into a helper/module with unit-testable behavior; `shell_input.c` wiring preserved, **no user-visible input/UX behavior change** (regression-tested by existing behavior expectations and host tests including `test_shell_input`).
- [x] `which <name-or-path>` implemented: resolves via same rules as `shell_script` resolver (CWD → `path` dirs → `/flash/scripts`, explicit paths with/without `.be` behavior consistent with current resolution), prints resolved virtual path or a clear “not found” result; does **not** execute the script.
- [x] Resolver/not-found messaging improved where in scope to list searched locations (explicit searched path, or CWD + `path` list + `/flash/scripts` as applicable) **without** breaking existing contract (explicit missing script still reported as handled/script-not-found style; bare-name miss remains unhandled/`Unknown command` path unchanged).
- [x] Automated checks per `AGENTS.md` for affected scope pass. Local: `make format-check`, `make test` (31/31 CTest, Python script tests 3/3). CI (run 44759321767, green): format-check, ESP-IDF Build & Lint, Unit Tests, integration tests incl. `shell_fs`. Host/unit tests added/updated for key parser extraction and for `which`/resolver behavior.
- [ ] Minimal manual HW validation (serial console on CYD) when behavior changes: verify `path list/add/remove/clear`, run scripts from CWD/path dir/`/flash/scripts`, verify `which` and improved messages; quick BT keyboard sanity if input parsing code paths were materially touched (regression spot-check). **Pending human action** (tracked in PR #109 comment; no device validation run in this session).

## Validation plan
- **Automated:** `make format-check`, `make test`. Add/update host tests (e.g. `test/test_shell_script.c` and/or new unit tests for key parser helper). Run `make build`, `make lint`, `make docs-check` when ESP-IDF/clang-tidy/Doxygen scope requires (per `AGENTS.md`).
- **Hardware/manual (minimal, CYD, serial):** confirm `path` commands, script execution from CWD / configured `path` dir / `/flash/scripts`, `which` resolution for representative cases (bare name, explicit path, missing), and error messages list searched locations where applicable. If input parsing refactor touches editing paths, do a short BT keyboard sanity (arrows/backspace/history) and record result; otherwise note not required. Record blockers if hardware unavailable.

## Expected areas/files (rough)
- `components/shell/src/shell_input.c`, `components/shell/src/shell_input.h` (and new `shell_keyparse.*` if introduced)
- `components/shell/src/shell_script.c/.h`, `components/shell/src/shell_cmds.h` or command registration points for `which` (and any shared resolver helpers)
- `test/test_shell_script.c` and/or new host tests for key parser
- Docs/help text updates only if required by new command/messages (keep minimal)

## Dependencies / prerequisites
- R20 umbrella: [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (item c context).
- R9/R22 resolver behavior and `path` command semantics (`docs/plan/2026-11-12-berry-command-script-migrations.md`, `components/shell/src/shell_script.c`).

## Risks / constraints
- ESP32 memory/stack and shell input complexity; key-parse extraction must not change timing/behavior.
- Must preserve resolver contract (order, `.be` handling, explicit vs non-explicit, handled-error semantics). Error string changes only where approved in this item (improved messages mentioning searched locations) and must not produce misleading `Unknown command` for handled cases.
- **Shell-surface overlap:** `unblock-shell-fs-pipeline-and-qemu-hangs` (ready) may touch shell/VFS/FS/test areas for evidence-backed remediation; coordinate changes/rebase when editing shared shell files to avoid regressions or merge conflicts.
- No new dependencies required by design; avoid scope creep into listing/copy helpers.

## Open questions
- None (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A). If implementation reveals a concrete blocker, document it here and do not move to Ready.

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 1).
- 2026-11-18 (clarify-feature): reviewed `components/shell/src/shell_input.c/.h`, `shell_script.c/.h`, R20 backlog and R22 plan context. User choices recorded (1A–4A). Open questions resolved.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
- 2026-11-20 (Ponytail Audit, plan-conflict resolution, doc-only): added coordination/risk note for shell-surface overlap with `unblock-shell-fs-pipeline-and-qemu-hangs` (ready). No implementation/validation performed.
- 2026-11-20 (implementation, branch `feat/r20c-shell-script-workflow` from `origin/main@54c61c07`, Feature 4 sequence): extracted key/escape name parsing into `components/shell/src/shell_keyparse.{c,h}` and wired `components/shell/src/shell_input.c` to use it (no UX behavior change intended; existing `test_shell_input` behavior tests pass). Added `which <name-or-path>` in `components/shell/src/shell_script.c` (registered in `shell_script_init`) resolving via the same token/resolver rules as script execution (CWD → configured `path` dirs → `/flash/scripts`; explicit path/.be behavior consistent) without executing scripts. Improved handled resolver/not-found messaging to report searched locations (explicit searched path, or CWD + configured `path` list + `/flash/scripts`) while preserving handled-vs-unknown semantics (bare-name miss remains unhandled → `Unknown command` path unchanged). Added/updated host tests: `test/test_shell_input.c` keyparse mapping coverage, `test/test_shell_script.c` `which` resolution/not-found/no-exec coverage. Build wiring updated (`components/shell/CMakeLists.txt`, `test/CMakeLists.txt`).
  - Local validation executed: `make format` (pass, exit 0), `make format-check` (pass, exit 0), `make test` (pass, exit 0; 31/31 CTest targets passed including `test_shell_input` and `test_shell_script`, plus Python script tests `Ran 3 tests ... OK`).
  - Not executed (blockers): `make build` / `make lint` not run locally — ESP-IDF v6.1 environment not confirmed/sourced in this session (requires `IDF_PATH`/`IDF_TOOLS_PATH` and sourced IDF export, clang-tidy DB at `build/clang/compile_commands.json`). **CI covered build/lint successfully** on PR (run 44759321767). `make docs-check` not run locally (Doxygen/tooling not verified). Integration/QEMU not run locally.
  - Hardware/manual validation (CYD serial, BT keyboard sanity if input parsing paths materially affected): **not run** in this session (no device attached). Required before merge/release confidence for on-device behavior: verify `path list/add/remove/clear`, script execution from CWD/path dir/`/flash/scripts`, `which` for bare/explicit/missing cases and searched-location messages; optional quick BT keyboard sanity (arrows/backspace/history) as a regression spot-check for key parsing wiring.
  - Delivery/CI: PR [#109](https://github.com/gallayl/cos/pull/109) opened (draft→ready), head `f1b15a5dc9681c284eab6dceefebfea103307c9a`. CI run [44759321767](https://github.com/gallayl/cos/actions/runs/44759321767) completed **success** with required checks green (incl. format-check, ESP-IDF Build & Lint, Unit Tests, all integration test suites including `Integration Tests (shell_fs)`). Branch up to date with `main` base at delivery time.
  - Independent self-review (evidence-based, read-only pass over diff): key/escape mapping extraction preserves existing mappings/behavior; `which` uses same resolver/token rules without execution; handled resolver errors/not-found messages include searched locations while bare-name misses remain unhandled (no spurious `Unknown command` for handled cases). No actionable findings identified in scope.
  - Remaining human actions: review/approval and merge (human). Hardware/manual validation pending and tracked as merge-blocking in PR comment [#issuecomment-7042839411](https://github.com/gallayl/cos/pull/109#issuecomment-7042839411) until resolved with evidence or explicitly waived by a human with rationale. Moving to `done/` records delivery/evidence and does **not** imply merge or completed hardware validation.
