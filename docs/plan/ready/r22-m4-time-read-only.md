# R22 M4 — Migrate `time` command to Berry script (read-only view)

- **ID:** r22-m4-time-read-only
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Ready:** 2026-11-15
- **Source/links:** [`../2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (M4), [`../needs-clarification/r22-command-script-migrations-open.md`](../needs-clarification/r22-command-script-migrations-open.md) (clarification decisions 2026-11-15), [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22), `components/time_sync/src/time_sync_cmd.c`, `components/time_sync/include/time_sync.h`
- **Approval:** User explicitly approved in chat on 2026-11-15 (“Approve and move to ready”). **Pre-approved for implementation/PR flow** (merge remains human/maintainer action). Scope includes global R22 decision to add `/flash/scripts` as built-in default resolver search directory.

## Open questions
- None. (Parent R22 M5 remains in [`../needs-clarification/r22-command-script-migrations-open.md`](../needs-clarification/r22-command-script-migrations-open.md), deferred.)

## Summary
Migrate the read-only shell `time` command to a Berry command script backed by a structured `system.time_info()` API, preserving current CLI contract/behavior. Clock synchronization remains native. Default script discovery improved by adding `/flash/scripts` as a built-in resolver search directory so stock migrated scripts work out-of-box.

## Scope
- Add structured Berry/native API: **`system.time_info()`** (module `system`, exposed consistently in Berry and TS SDK declarations/bindings as applicable) returning:
  - Success: `{ ok: true, synced: boolean, epoch: number, utc: string, local: string }`
    - `synced`: `time_sync_is_synced()`
    - `epoch`: `time_sync_get_epoch()`
    - `utc`: `time_sync_get_utc(...)` formatted string (e.g. `2026-02-18T14:30:00Z`)
    - `local`: `time_sync_get_local(...)` formatted string (e.g. `2026-02-18T15:30:00+01:00`)
  - Failure (unexpected format/error): `{ ok: false, error: string }` (ESP-IDF/stable error name)
- Add stock command script `storage/ts/scripts/time.ts` and compiled `storage/flash_data/scripts/time.be` implementing **strict parity** with current native `time`:
  - `time` / no args: print `Local:  <local>\nUTC:    <utc>\nSynced: yes|no\n`, return 0 on success.
  - `time utc`: print `<utc>\n`, return 0 on success.
  - `time epoch`: print `<epoch>\n` (decimal), return 0 on success.
  - Any other/unknown subcommand: same as full status (current fallthrough behavior preserved).
  - On `time_info` failure/format error: print `time: format error (<error>)\n`, return 1.
- Remove native `time` console registration only after script + resolver + API + validation pass for this command (per R22 guardrails).
- Resolver discoverability (global R22 decision): include **`/flash/scripts` as a built-in default search directory** so stock scripts work with empty `/flash/.cos_path`; document and test precedence with CWD and configured `path` entries.
- Update README/help/docs/SDK/binding inventory/manifest as required for new API and migrated command.

## Non-goals
- Changing or migrating clock synchronization behavior/APIs/commands (sync remains native).
- Parsing console output via `action()`.
- Migrating M5 (`ls`/`dir`) or any command not covered by M4.
- Requiring QEMU/NTP environment for validation.

## Acceptance criteria
- [ ] `system.time_info()` implemented with structured results/errors as specified; no console scraping.
- [ ] `time`, `time utc`, `time epoch` script behavior and exit codes strictly match current native contract, including unknown-arg fallthrough and format-error output.
- [ ] Native `time` registration removed only after script is present, resolvable by default, and validation passes.
- [ ] `/flash/scripts` is searched by default by resolver (stock commands discoverable out-of-box); precedence documented and tested.
- [ ] Automated checks and evidence recorded per Validation plan; on-device spot-check evidence recorded for time outputs/sync-state behavior and sync-native unaffected.
- [ ] Docs/plans updated (`docs/plan/2026-11-12-berry-command-script-migrations.md`, R22 consolidated tracker links/evidence as applicable).

## Validation plan
- **Automated (required per scope/AGENTS):** `make format-check`, `make test`. Firmware/build/lint: `make build` and `make lint` when feasible in ESP-IDF v6.1 env. `make docs-check` if public docs/Doxygen scope affected. Frontend/SDK: if `frontend/src/cos-sdk/` or IDE/bindings/manifest changed, from `frontend/` run `yarn format:check`, `yarn lint`, `yarn test`, `yarn build`.
- **Host/unit tests where feasible:** `system.time_info` binding behavior/wiring, script arg handling and output formatting/parity logic, resolver default `/flash/scripts` behavior/precedence.
- **Integration/QEMU:** targeted additions only where deterministic/feasible (not required for NTP/time sync).
- **On-device/hardware spot-check (required):** verify `time`, `time utc`, `time epoch` with recorded sync state(s) as available (synced/not-synced), confirm output labels/spacing/values format and exit behavior parity, confirm sync remains native/unaffected. Record firmware/commit, exact commands/output, and environment notes in linked report/evidence.
- **Reporting:** run relevant checks proactively, report exact commands and pass/fail; do not claim unexecuted checks passed; document blockers (e.g. missing ESP-IDF env, hardware unavailable) concretely.

## Expected areas/files
- `components/time_sync/` (`time_sync.h/.c` as needed for API support; `time_sync_cmd.c` native registration removed after migration)
- `components/berry_lang/src/` (native binding for `system.time_info`, module registration as needed)
- `components/shell/src/shell_script.c` and/or shell script resolver/path logic (default `/flash/scripts`), related headers/tests
- `frontend/src/cos-sdk/system.d.ts`, binding inventory/manifest/IDE registration files as required
- `storage/ts/scripts/time.ts`, `storage/flash_data/scripts/time.be`
- `test/` (host/unit), `integration_test/` (targeted, if feasible), `README.md`/docs/help text as applicable
- `docs/reports/` (hardware evidence), `docs/plan/2026-11-12-berry-command-script-migrations.md`, consolidated plan updates

## Dependencies / prerequisites
- Existing `time_sync_init/is_synced/get_epoch/get_utc/get_local` APIs and native time sync behavior remain available.
- Berry/SDK `system` module extension consistent with existing `system.uptime()`/`system.meminfo()` patterns.
- R9 resolver/`__cos_entry` command-script execution and stock script deployment conventions.

## Risks / constraints
- SNTP/network-dependent state: `synced`/time values vary; validation must record observed state rather than assume sync.
- Berry numeric/epoch representation follows existing runtime conventions; script must print epoch in parity-compatible decimal form.
- Strict parity includes unknown-arg fallthrough to full status and exact user-visible formatting/spacing.
- Default `/flash/scripts` search changes resolver behavior; must preserve explicit-path, CWD, and configured-path expectations and be covered by tests.

## Progress / evidence
- 2026-11-15: clarified and drafted from `docs/plan/needs-clarification/r22-command-script-migrations-open.md` (user decisions: M4 only now, `system.time_info()`, strict parity, default `/flash/scripts`, host/automated + on-device spot-check). Explicitly approved and moved to `docs/plan/ready/` as pre-approved for implementation/PR flow (merge remains human).
- 2026-11-15: re-reviewed against `clarify-feature` Ready contract and relevant context (`components/time_sync/src/time_sync_cmd.c`, resolver/SDK/module patterns). **No open questions or spec gaps identified; remains `ready` (approved) in `docs/plan/ready/`.** No implementation performed.
