# Berry Command-Script Migrations (Follow-on Plan)

**Created:** 2026-11-12  
**Status:** Planned (not started)  
**Owner/tracking:** Tracked as **R22** in [`docs/plan/consolidated-improvement-plan.md`](./consolidated-improvement-plan.md) (canonical status/evidence). This document provides implementation planning details.  
**Source:** [`docs/history/2026-11-01-improvements.md`](../history/2026-11-01-improvements.md) “Follow-on command refactors”, R9 resolver/`path`/pilot infrastructure (complete; native `ping` migrated 2026-11-12).

## Summary

Migrate selected native shell commands to Berry command scripts in a controlled, API-gated order. Each migration must preserve the existing user-visible command contract unless an explicit, documented improvement is approved, and must rely on structured native Berry APIs (not parsing human-readable console output via `action()`).

This is a **migration order/proposal promoted to planned work**, but individual command migrations remain gated by prerequisites and validation evidence. A command may remain native even after Berry gains access to the same underlying service if migration is not justified.

## Goals

- Reduce native command surface where behavior is well-served by script-level argument handling/output and a stable structured native API exists.
- Keep resolver semantics and command contracts predictable (CWD/`path` lookup, native precedence for still-registered commands, explicit-path behavior, `main(args)`/entrypoint contract per R21).
- Preserve or improve testability by exposing typed/structured Berry APIs for migrated behavior.
- Maintain safety on constrained hardware (no avoidable heap churn/regressions; measure when behavior changes memory/network/runtime characteristics).

## Non-goals

- Migrating commands that are core shell navigation/state mutation, destructive/system-control, or hardware/interactive/async/lifecycle-heavy unless a specific Berry API and UX justification is approved.
- Making scripts call `action()` and parse human-readable console output to obtain structured results.
- Changing R9 resolver precedence/order/tokenization/`path` persistence semantics except as explicitly required and documented.
- Bulk-removing native implementations before pilot/contract validation for each migrated command.

## Global prerequisites and guardrails

1. **Structured API first:** For each command, expose/verify a native Berry API with typed/structured results and errors sufficient for the script to implement the command without scraping console text.
2. **Contract preservation:** Preserve usage, argument parsing/bounds/defaults, success/failure output shape, and exit/status semantics as currently documented/validated (or record intentional diffs in README/help/plan evidence).
3. **Discovery:** Stock scripts must be installed in a location discoverable by default/user-expected flows (typically `/flash/scripts/*.be` via CWD/`path` conventions) and documented. Consider `path` defaults/bootstrap only with explicit decision (not assumed here).
4. **Native registration removal timing:** Remove/make unreachable native registration only after the script is present, resolver lookup is active, and target+host validation (including pilot/explicit-path checks) pass for that command.
5. **Tests:** Add/update host unit tests for command contract/parsing where feasible, script-resolution/dispatch coverage for migrated names, and Berry script/API tests as available; add QEMU/integration coverage when practical.
6. **Validation/evidence:** Follow `AGENTS.md` DoD; run relevant checks proactively, report exact commands/outcomes, and record hardware validation when required (network/sensor/time/UI/hardware-dependent behavior).

## Migration order and per-command plans

### M1 — `nslookup` (Priority 1, natural companion to `ping`)

- **Current state:** Native command in `components/shell/src/shell_network_cmds.c` (`nslookup <host> [timeout_ms]`, uses `network_resolve`); `ping` already script-migrated and uses `net.ping`.
- **Prerequisite API:** Expose `net.resolve(host, timeout_ms)` (or equivalent) in Berry with structured address/error results matching current resolver behavior/contracts (including timeout bounds/defaults and error propagation). Reuse existing network/worker constraints (serialized worker behavior as applicable) without regressing safety.
- **Script:** Add `storage/flash_data/scripts/nslookup.be` (and TS source `storage/ts/scripts/nslookup.ts` if maintained) preserving output/usage contract (e.g. `Name: <host>` / `Address: <ip>`, failure `nslookup: <host> failed (<err>)`, timeout-arg validation consistent with bounds).
- **Migration:** After API + script + tests + validation, remove native `nslookup` registration and update README/help/docs.
- **Acceptance criteria:** Bare `nslookup` resolves to script with preserved contract; invalid args/timeouts/resolve failures behave correctly; no regression to `ping`/network worker behavior; host tests pass; on-device Wi‑Fi/DNS validation recorded.
- **Risks:** DNS/timeout behavior under load/serialized worker edge cases; ensure API exposes enough detail for script parity.

### M2 — `uptime`, `info`, `mem` / `memory` (Priority 2, read-only system commands)

- **Current state:** Native implementations provide read-only system/heap/runtime info; `mem` and `memory` expose the same operation (migrate together).
- **Prerequisite API:** Expose typed system/heap information in Berry (uptime ticks/seconds, firmware/build/info fields as currently shown, heap/current/min/largest/total as applicable) via stable structured APIs. Avoid parsing logs/console output.
- **Scripts:** `uptime.be`, `info.be`, `mem.be` (and/or `memory.be` alias/script naming decision) preserving current output formats where feasible or documenting intentional, user-approved formatting changes.
- **Migration:** Migrate together for `mem`/`memory` parity; can be staged per command if APIs land incrementally, but each removal requires contract validation.
- **Acceptance criteria:** Commands produce equivalent information and exit behavior; scripts do not require console scraping; host tests cover formatting/argument handling where applicable; hardware spot-check confirms values reasonable.
- **Risks:** Some info may be easier/cheaper to read natively; ensure Berry APIs do not add notable allocation/fragmentation overhead on constrained device.

### M3 — `light` (Priority 3, read-only sensor)

- **Current state:** Native sensor command with hardware dependency.
- **Prerequisite API:** Native Berry API exposing sensor reading and error/unavailable state.
- **Script:** `light.be` preserving CLI arguments and presentation/error contract.
- **Acceptance criteria:** Correct readings/error handling on hardware; graceful behavior when sensor unavailable; host tests for arg handling/script resolution where feasible.
- **Risks:** Hardware-dependent validation required; API must reflect sensor availability/state clearly.

### M4 — `time` (Priority 4, read-only view)

- **Current state:** Native `time` command shows local/UTC/sync/epoch as applicable; clock synchronization is separate.
- **Prerequisite API:** Structured Berry API for time and synchronization state (read-only). **Keep clock synchronization itself native.**
- **Script:** `time.be` preserving read-only subcommands/output/status behavior; do not move sync control into script unless separately designed and approved.
- **Acceptance criteria:** Read-only time display/sync status parity; sync operations remain native and unaffected.
- **Risks:** Timezone/formatting/sync-state edge cases; hardware/network/NTP context may be needed for full validation.

### M5 — `ls` / `dir` (Priority 5, filesystem listing example)

- **Current state:** Native filesystem listing commands; `fs.list` exists but must have a complete, stable contract for paths, entry types, and errors for parity.
- **Prerequisite API:** Complete/stable Berry filesystem listing API contract (paths, entry metadata/types, errors, traversal/access-control behavior expectations). Keep filesystem traversal and access controls native.
- **Scripts:** `ls.be` and/or `dir.be` preserving aliases/flags/output expectations currently supported (or explicitly scoping supported subset with documented diff).
- **Acceptance criteria:** Listing parity for supported cases (virtual paths, CWD-relative, flash/sd as applicable), correct error handling, no unsafe traversal exposure; host + integration/QEMU coverage where feasible; hardware spot-check for SD/flash behavior.
- **Risks:** Highest parity/UX risk in this batch (flags, formatting, permissions/errors, large dirs/memory). Consider staged delivery.

## Commands to keep native for now (unless separately justified)

Per source rationale (revalidate at implementation time):
- **Core shell/file mutation & navigation:** `cd`, `pwd`, `mkdir`, `rm`, `cp`, `mv`, `rmdir`, `touch`, SD mount management.
- **System control/runtime:** `restart`, `format`, command/runtime management.
- **Interactive/async/hardware/lifecycle:** `calibrate`, `wifi`, `bt`, `i2c`, `server`, `ws`, `screen`, `display_mode`.

## Implementation sequence (recommended)

1. **M1 `nslookup`** — validates second network command migration on existing worker/API patterns.
2. **M2 `uptime`/`info`/`mem`/`memory`** — low-risk read-only commands once system APIs are exposed.
3. **M3 `light`** — small read-only sensor command (hardware validation).
4. **M4 `time`** — read-only view after sync-state API is clear (sync remains native).
5. **M5 `ls`/`dir`** — only after `fs.list` contract parity/stability is confirmed; scope carefully.

Stages can be split into separate PR-sized changes: (a) Berry API + tests, (b) stock script + docs, (c) remove native registration + final validation, when that reduces risk.

## Validation checklist (per migration)

- [ ] Structured Berry API implemented and documented (no console-output parsing).
- [ ] Stock `.be` script (and TS source if applicable) added/updated; contract preserved or diffs documented/approved.
- [ ] Native registration removed only after script+resolver validation for that command.
- [ ] Automated tests added/updated (host/unit and/or integration/QEMU as applicable).
- [ ] `make format-check` and `make test` pass; additional checks run per `AGENTS.md` scope (firmware build/lint, docs-check, integration tests, frontend checks if affected).
- [ ] Memory/runtime-sensitive changes measured when relevant.
- [ ] Hardware validation recorded when behavior depends on physical device/network/sensors/time.
- [ ] README/help/plan progress and evidence updated.

## Progress tracking

| ID | Command(s) | Priority | Status | Prerequisites / notes |
|---|---|---:|---|---|
| M1 | `nslookup` | P1 | ⬜ Not started | Needs `net.resolve` (or equivalent) Berry API; preserve timeout/output/error contract; on-device DNS validation |
| M2a | `uptime` | P1 | ⬜ Not started | Needs typed system uptime Berry API |
| M2b | `info` | P1 | ⬜ Not started | Needs typed system/info Berry API |
| M2c | `mem`, `memory` | P1 | ⬜ Not started | Needs typed heap/system Berry API; migrate together |
| M3 | `light` | P2 | ⬜ Not started | Needs sensor reading/error Berry API; hardware validation |
| M4 | `time` (read-only view) | P2 | ⬜ Not started | Needs time/sync-state Berry API; keep sync native; hardware/NTP context as needed |
| M5 | `ls`, `dir` | P2 | ⬜ Not started | Needs complete/stable `fs.list`/listing API contract; scope/format parity carefully; may stage |

## Open questions (resolve before/during implementation)

- Default discoverability: should `/flash/scripts` (or specific dirs) be added to default `path`/bootstrap so migrated commands work out-of-the-box on stock images without manual `path add`?
- Exact Berry API names/signatures and error schemas for system/info/time/sensor/resolve (`net.resolve` naming/shape, consistency with `net.ping`).
- Acceptable formatting differences (if any) vs strict byte-for-byte output parity; document per command.
- Whether `memory` remains a separate script/alias/compat name alongside `mem` after migration.
- Integration-test/QEMU coverage strategy for network/DNS and hardware-dependent commands.
- Staging/granularity for M5 (`ls`/`dir` flags and output parity scope).

