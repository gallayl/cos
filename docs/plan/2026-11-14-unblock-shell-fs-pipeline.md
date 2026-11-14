# Unblock shell/FS integration pipeline (refactor + unit coverage)

**Created:** 2026-11-14  
**Status:** In progress (Phase 0 started)  
**Branch/PR:** `feat/r14-ts-diags` / [#22](https://github.com/gallayl/cos/pull/22)  
**Scope:** Stabilize and unblock `Integration Tests (shell_fs/init_sequence/system)` on CI by reverting speculative runtime guesses, then front-loading deterministic shell/FS behavior into host unit tests and adding minimal seams only where evidence requires it.  
**Related:**
- [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](2026-11-14-unit-test-coverage-and-integration-rebalance.md) — detailed inventory for R25 #11 (P1 VFS/shell-fs semantics, integration→unit rebalancing)
- [`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md) — R25 #4 (testability seams, console transport/task/driver), #11 (coverage enablement), #2 (startup composition)
- `AGENTS.md` — validation/reporting expectations

## Problem / evidence summary

- **Original `shell_fs` failures** (run [43588554529](https://github.com/gallayl/cos/actions/runs/43588554529)): runtime stalls in vendored LittleFS/FS command-time open/write paths (not test encoding); `28 passed / 3 failed` (`test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rm_file`).
- **Subsequent churn (2026-11-14):** a series of speculative `components/shell/src/shell_input.c` UART/VFS/driver install-probe/reattach/lazy-install/backoff changes regressed boot/init in QEMU (`init_sequence`/`system` timeouts, UART driver errors/log floods, init hangs at various attach/probe points). These attempts did not establish a confirmed fix for the original `shell_fs` stalls.
- **Constraint:** no local ESP-IDF v6.1/QEMU environment; `make build`/`make lint`/integration validation depends on CI. Host checks available locally: `make format-check`, `make test`.

## Goal

Unblock the PR pipeline by **refactoring problematic areas and increasing host unit-test coverage** so deterministic failures are caught early, while keeping integration tests for true platform/runtime behavior (REPL/console transport, mounted LittleFS, command registration/startup, Berry-on-target, async console).

## Non-goals

- Further trial-and-error UART/console init tweaks without evidence and tests.
- Weakening integration assertions except where explicitly rebalanced to an equivalent host test.
- Broad R25 work (startup decomposition, unrelated Berry/UI/network refactors, vendored `esp_littlefs` rewrite) unless required by evidence from this pipeline.

## Baseline / revert

- **Baseline:** `273115c` (`fix(shell): restore UART VFS attach for console-owned driver`) for shell input + related host mocks — pre-churn state before driver install/probe/backoff guessing. Known: `273115c` alone was not proven green for later stalls; used here as a clean baseline, not a claimed fix.
- **Revert commit (local, created before this plan doc):** `ab11e98` — `revert(shell): restore shell input and UART mocks to 273115c baseline`
  - Restores: `components/shell/src/shell_input.c`, `test/mocks/driver/uart.h`, `test/mocks/mock_uart.c`, `test/mocks/freertos/task.h`, `test/mocks/mock_freertos_extra.c`
- **Legitimate test fixes retained in history:** `e8a70e1`, `7a69991`, `cf4d242` (`integration_test/test_shell_fs/test_shell_fs.py`).

## Progress tracking

| Phase | Commit(s) | Status | Notes |
|---|---|---|---|
| Plan doc + tracking | `37075c2` | Done | Persist plan, cross-refs, baseline |
| 0 — Baseline gate | `ab11e98`, `37075c2` (pushed) | In progress | Revert+plan pushed; observing CI run [43613402861](https://github.com/gallayl/cos/actions/runs/43613402861) to classify failures (A transport/init, B VFS/LittleFS runtime, C deterministic logic) |
| 1 — Shell FS command unit tests (quick win) | (this phase commit) | Done (host) | Strengthened `mock_filesystem` stream/copy behavior and added host `shell_fs_cmds` tests (`cat` read/missing/dir, `rm` missing, `touch` preserves content, `cp` preserves content). `make format-check`/`make test` pass locally; CI integration impact pending |
| 2 — VFS contract tests / minimal seam + targeted fix | TBD | Pending | `vfs.c` contract tests (mocked/seamed backend as needed); evidence-backed fixes; keep real LittleFS integration |
| 3 — Console/input seam (only if needed) | TBD | Pending | Minimal seam for `shell_input` init/read policy if Phase 0 shows transport/init blocking; no init guesswork |
| Pipeline green | — | Pending | Required PR checks green; root causes evidence-backed |

## Phase details

### Phase 0 — Baseline gate
1. Push `ab11e98` and this plan doc; observe CI once to re-establish clean failure signatures on reverted baseline.
2. Download failed logs/artifacts and classify:
   - **A:** shell input/console transport boot (init hangs, UART driver/read errors, missing readiness/prompt)
   - **B:** VFS/flash/LittleFS command-path runtime (mounted FS stalls/errors)
   - **C:** deterministic command/arg/output/path logic suitable for host tests
3. Record findings in this document and choose next evidence-backed step (prioritize C via Phase 1, B contract/runtime split via Phase 2, A via Phase 3 only if blocking).

**Validation:** CI observation + artifact review (no local QEMU). Report run/job IDs and signatures.

### Phase 1 — Shell FS command semantics (host unit tests)
*Aligns with [coverage/rebalance plan](2026-11-14-unit-test-coverage-and-integration-rebalance.md) P1 “Shell FS command semantics” and quick wins.*

- Add/extend host tests invoking `components/shell/src/shell_fs_cmds.c` command handlers against `test/mocks/mock_filesystem` (and `vfs_path`/console mocks as needed) for success/error paths: `ls`, `cat`, `touch`, `mkdir`, `cd`, `pwd`, `rm`, `rmdir`, `cp`, `mv`, `hexdump`, `info` (as applicable), including nonexistent targets, dir-not-empty, overwrite/copy rules, relative resolution.
- Prioritize deterministic assertions related to original `shell_fs` failures: `rm` file behavior, `cp` copies without overwriting, and command-layer aspects of alias lifecycle where applicable.
- **Rebalance:** when a behavior is covered by host tests, keep integration focused on REPL/prompt/editing, UART/console transport, mounted LittleFS, registration/startup, Berry-on-target, async console. Update `integration_test/test_shell_fs` only if/when assertions are explicitly moved with equivalent confidence (track here).

**Validation:** `make format-check`, `make test` (host). CI for integration impact.

### Phase 2 — VFS contract tests / minimal seam + targeted fix
*Aligns with P1 “VFS high-level API behavior”.*

- Add host contract tests for `components/filesystem/src/vfs.c` (and coordination with `flash.c`/path rules): arg handling, path normalization, error mapping, file/dir semantics, overwrite/list/exists. Introduce the smallest seam/mocked backend needed for contracts not covered by existing mocks.
- If Phase 0/CI shows contract/logic defects, fix in VFS/shell-fs layer with unit coverage. If failures are vendored LittleFS/mount/runtime-specific, make narrow evidence-backed changes and validate in integration; document limits.

**Validation:** `make format-check`, `make test`; CI integration.

### Phase 3 — Console/input seam (only if Phase 0 shows A blocking)
*Aligns with R25 #4 (console transport/task/driver seams), narrowly scoped.*

- Introduce a minimal injectable seam for `shell_input` transport/init/read policy (default behavior unchanged) to make init/error decisions host-testable.
- Implement evidence-backed fix for the specific blocking boot issue with unit tests for seam logic + integration confirmation. **No** reintroduction of speculative install/probe/VFS-reattach loops in init.
- `init_sequence`/`system` remain E2E; extract only pure decisions if necessary.

**Validation:** `make format-check`, `make test`; CI integration.

## Execution rules

- **One commit per phase** (plan doc/setup may be its own commit; revert already committed as `ab11e98`).
- Run relevant host checks proactively after changes (`make format-check`, `make test`); fix and re-run until pass. Report exact commands/outcomes.
- Do not claim build/lint/integration pass without environment/CI evidence; explain blockers and give actionable rerun steps per `AGENTS.md`.
- Update this document’s progress table/notes as phases complete or when CI evidence changes direction.

## CI observations

- **Phase 0 push (baseline revert `ab11e98` + plan `37075c2`)** — Integration run [43613402861](https://github.com/gallayl/cos/actions/runs/43613402861), Build/Lint [43613402894](https://github.com/gallayl/cos/actions/runs/43613402894): in progress at time of Phase 1 commit. Non-integration PR checks observed passing (Format, Doxygen, Unit Tests, Frontend/Lint-format-test-build). Failure signatures/artifacts to be recorded when complete.

## Skipped / risks (living)

- `273115c` baseline may still exhibit integration failures; runtime FS/transport issues may require Phase 2/3 evidence-backed work beyond unit tests.
- Over-rebalancing can drop needed platform coverage — keep REPL/console/mounted-FS/Berry-on-target paths in integration.
