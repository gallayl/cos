---
name: validate-change
description: Run and report scope-appropriate validation for the current change with evidence-based outcomes. Use as a granular procedure (composed by primary workflows) to execute required checks, fix failures, and report exact commands, pass/fail results, and actionable blockers.
---

# Validate change (granular)

Run relevant validation proactively for the affected scope, fix failures, and re-run until passing. This is a **granular skill** composed by primary entry procedures (e.g. `implementation-workflow`, `testing`)—do not duplicate conflicting policy; follow root `AGENTS.md` ("Definition of done") as authoritative.

## Rules

- **Never claim a check passed unless it was executed.** Report exact commands and pass/fail outcomes; mention fixes applied and re-runs.
- **Do not rely on CI.** If a check cannot run or fails due to environment/tooling, explain the concrete blocker and give actionable remediation (what to install/configure, versions, env vars/paths such as `IDF_PATH`/`IDF_TOOLS_PATH`/`PATH` or Node 24.x + Yarn, and the exact rerun command).
- Prefer Makefile targets for firmware/host checks and frontend `yarn` scripts in `frontend/` to stay aligned with CI.

## Default checks (most changes)

- `make format-check` — formatting must match CI.
- `make test` — host-based unit tests.

If formatting fails, run `make format`, then re-run `make format-check`.

## Additional checks by scope (run when applicable)

| Scope | Checks | Notes |
| --- | --- | --- |
| Frontend / `storage/ts` / `storage/flash_data` | From `frontend/`: `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` | Matches CI (Node 24.x, Yarn). If deps missing/stale: `yarn install --frozen-lockfile` in `frontend/`. |
| Firmware / build / lint-covered C/C++ | `make build`, and `make lint` when feasible | Requires ESP-IDF v6.1 environment sourced; clang-tidy uses `build/clang/compile_commands.json`. |
| Documentation / Doxygen comments | `make docs-check` | Must pass with zero warnings. |
| Integration-test-affected behavior / QEMU | `make integration-test-build` and/or `make integration-test` (or `make integration-test-run`) | Requires ESP-IDF v6.1, QEMU, and deps per README/CI. |

## Output

- Summarize results per command (pass/fail) and any environment-limited gaps.
- For hardware-only validation, run everything possible locally/automatically and clearly state what remains for on-device validation.
