# R22 M5 — Migrate `ls`/`dir` commands to Berry scripts

- **ID:** r22-m5-ls-dir-command-scripts
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Updated:** 2026-11-17
- **Source/links:** [`../2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (R22, M5), [`../done/r22-command-script-migrations-open.md`](../done/r22-command-script-migrations-open.md) (clarification tracker, M5 spec/decisions), [`../done/r22-m4-time-read-only.md`](../done/r22-m4-time-read-only.md) (M4 done; on-device spot-check user-confirmed 2026-11-16), [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22). Related: R9/R21 command-script resolver/entrypoint contract, `fs.list` Berry API.
- **Approval:** M5 spec explicitly approved by user in chat on 2026-11-15 (draft accepted as final; no corrections 2026-11-17). **2026-11-17 (user chat):** user accepted existing M4 on-device spot-check record as satisfying the recorded gate and explicitly approved moving M5 to ready now. **Pre-approved for implementation/PR flow** per `global-gates` (merge remains human/maintainer action).
- **Open questions:** none

## Summary
Migrate native shell `ls`/`dir` filesystem listing commands to Berry command scripts backed by the existing structured `fs.list` API (additive API changes only if a concrete gap is confirmed). Scripts must follow the approved M5 contract: `-a`/`-l`/`-la`/`-al`, real file sizes (intentional improvement over native readdir fast-path 0-size quirk), deterministic name-sorted output, defined root/error behavior, and conservative truncation warning when `fs.list` returns the 64-entry cap. `/flash/scripts` default resolver behavior (M4 prerequisite) is already in place.

## Scope
- Add stock command scripts `storage/ts/scripts/ls.ts` and `storage/ts/scripts/dir.ts` and compiled `storage/flash_data/scripts/ls.be` and `storage/flash_data/scripts/dir.be` (or equivalent shared implementation) implementing **identical args/output/exit behavior** for `ls` and `dir`.
- **Usage/args:** `ls [-a] [-l] [path]` and `dir [-a] [-l] [path]`. Flags may appear before path as `-a`, `-l`, `-la`, or `-al`. `path` defaults to CWD when omitted. At most one path argument. Unknown flags, extra args, or invalid flag forms print `Usage: ls [-a] [-l] [path]` (or `dir ...` matching invoked name) and exit 1.
- **`-a`:** include entries whose names start with `.` if returned by listing; `.` and `..` are never shown (VFS omits them). Behavior specified for correctness/forward-compat if dotfiles are ever returned.
- **`-l` (simple long format):** one row per entry: `<type> <size> <mtime> <name><suffix>` where `<type>` is `d` for directory else `-`, `<size>` is decimal bytes (real size from `fs.list`; directories may show 0), `<mtime>` is decimal `mtime` seconds (`fs.list` value; 0 if unavailable), `<name>` is entry name and `<suffix>` is `/` for directories else empty. Columns separated by single spaces. Summary line after entries: `%u file(s), %u dir(s), %u bytes` (bytes summed from file sizes shown); empty dir prints `(empty)`.
- **Default (no `-l`):** preserve native style but with **real sizes** (intentional diff from native readdir fast-path 0-size quirk, documented): directories `  <DIR>  <name>/`, files `  %5u  <name>` using actual file size (right-aligned in >=5 chars as today). Summary/empty behavior as current.
- **Ordering:** entries sorted by name (deterministic, byte/string compare) before printing for both plain and `-l` views.
- **Root `/`:** list available filesystems as today (`flash/`, `sdcard/` when mounted) adapted to selected format and real/known metadata (size/mtime 0 if not applicable), with matching summary counts/bytes; synthetic entries sorted by name (`flash`, `sdcard` when present).
- **Errors/exit:** invalid/unresolvable path -> `ls: invalid path` and exit 1; cannot open/list/not-a-directory/access error -> `ls: cannot open '<path>'` (or equivalent stable message matching native intent; exact parity validated in implementation/tests) and exit 1. Usage errors exit 1. Success exits 0.
- **Truncation (64-entry cap, `VFS_ENTRIES_MAX`):** if `fs.list` returns `count == 64`, show listed entries (sorted subset as returned/processed) and print warning `ls: warning: listing may be truncated at 64 entries` after output/summary; **exit 0**. Conservative because `fs.list` does not indicate overflow beyond cap; additive overflow-detection API deferred unless required.
- **API sufficiency:** current `fs.list` (`name,size,is_dir,mtime,ctime`, max 64, structured ok/error) is expected sufficient. Confirm during implementation (no additive API/behavior changes planned). If a concrete gap is found, re-clarify before implementing non-spec behavior/API changes.
- **Native removal timing:** remove native `ls`/`dir` console registration only after scripts are present, resolvable by default (`/flash/scripts`), and validation passes (per R22 guardrails).
- Update README/help/docs and expected script/command lists/manifests as required for migrated commands and intentional diffs (real sizes, truncation warning, supported flags).

## Non-goals
- Migrating commands beyond `ls`/`dir` or other R22 milestones.
- Parsing console output via `action()` or using non-structured sources for listing results.
- Changing filesystem traversal/access controls (remain native/enforced by VFS/FS APIs).
- Additive `fs.list` overflow-detection/API changes unless a concrete, confirmed gap requires re-clarification.
- Clock sync, network, or unrelated shell/runtime changes.

## Acceptance criteria / definition of done
- [ ] `storage/ts/scripts/{ls,dir}.ts` and compiled `storage/flash_data/scripts/{ls,dir}.be` implement the approved M5 contract (args/flags, `-a`/`-l`/combined flags, real sizes, deterministic name sorting, plain/long formats, root handling, usage/error/exit behavior, truncation warning on count==64 with exit 0) with identical behavior for `ls` and `dir`.
- [ ] Native `ls`/`dir` console registration removed only after scripts are present, resolvable by default (`/flash/scripts`), and validation passes.
- [ ] `fs.list` sufficiency confirmed during implementation; no undocumented API/behavior changes. Any discovered concrete gap triggers re-clarification before deviating from this spec.
- [ ] Host/unit tests added/updated for arg parsing, formatting (plain/long), sorting, summary/empty, root handling as feasible, truncation warning (count==64), and error/usage/exit cases.
- [ ] Targeted QEMU/VFS/script-dispatch tests added/extended where deterministic/feasible for VFS listing/scripted command behavior (document gaps/limitations).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass and are reported with evidence: `make format-check`, `make test` (required). If firmware/C-C++ build/lint-covered code changes and ESP-IDF v6.1 env available: `make build`, `make lint`. If frontend/SDK/storage-TS artifacts or expected script lists/manifests change, run required frontend checks from `frontend/` (`yarn format:check`, `yarn lint`, `yarn test`, `yarn build`) and `npm run check-storage`/compile as applicable per repo conventions. If integration tests change and env available, build/run affected suite(s).
- [ ] On-device spot-check performed when hardware/environment available: `ls`/`dir` on `/flash` and `/sdcard` (if available) including `-a`, `-l`, `-la`/`-al`, path/default-CWD behavior, root `/`, and truncation behavior as reproducible; record firmware/commit, commands/output, and environment notes. If unavailable, record concrete blockers/unverified items per `AGENTS.md`.
- [ ] README/help/docs and plan tracking updated (`docs/plan/2026-11-12-berry-command-script-migrations.md`, consolidated plan R22/evidence, ready→done state on delivery) with intentional diffs documented (real sizes, truncation warning, supported flags).

## Validation plan
- **Automated (required):** `make format-check`, `make test`.
- **Automated (conditional by scope/env):** `make build`, `make lint` when firmware/C-C++ build/lint-covered scope changes and ESP-IDF v6.1 env available. Frontend/storage-script checks from `frontend/` when TS scripts, SDK/bindings/manifests, or compiled storage artifacts/expectations change (`yarn format:check`, `yarn lint`, `yarn test`, `yarn build`, and `npm run check-storage` / compile-storage flow as required). Targeted `integration_test` build/run when deterministic VFS/script-dispatch coverage is added/changed and env available.
- **Hardware/manual (when available):** on-device spot-check on `/flash` and `/sdcard` (if present): `ls`, `dir`, `ls -a`, `ls -l`, `ls -la`, `ls -al`, explicit path, invalid path/usage, root `/`, and truncation case if reproducible (directory with >=64 entries) to observe warning/exit 0. Record evidence; if unavailable, state blockers and unverified items per `AGENTS.md`.

## Expected areas/files
- `storage/ts/scripts/ls.ts`, `storage/ts/scripts/dir.ts`
- `storage/flash_data/scripts/ls.be`, `storage/flash_data/scripts/dir.be`
- `components/shell/` (remove native `ls`/`dir` registration after validation; help text if applicable)
- `components/filesystem/` and/or Berry `fs.list` bindings only if an implementation-time gap is confirmed and re-clarified (not planned)
- `frontend/tools/compile-storage.test.ts` and storage compile/manifest expectations as applicable
- `test/` host/unit tests for script/command behavior or helpers as feasible
- `integration_test/` targeted VFS/script-dispatch coverage where deterministic/feasible
- `README.md`, shell help/docs as applicable
- `docs/plan/2026-11-12-berry-command-script-migrations.md`, `docs/plan/consolidated-improvement-plan.md`, this ready item and eventual `docs/plan/done/` move with evidence

## Dependencies / prerequisites
- R9/R21 command-script resolver/entrypoint contract and stock script deployment conventions.
- **M4 `/flash/scripts` default resolver search** landed and in use (R22 prerequisite). M4 on-device spot-check evidence accepted by user on 2026-11-17 as satisfying M5 gate (M4 human merge may remain pending separately).
- `fs.list` Berry API contract as currently implemented (`name,size,is_dir,mtime,ctime`, max 64 entries, structured ok/error). Implementation-time sufficiency check required; re-clarify if additive changes needed.
- Supported hardware/firmware image with `/flash` and optional `/sdcard` for on-device validation when available.

## Risks / constraints
- **Parity/UX risk:** output formatting, flags, error strings, root behavior, and large-directory handling can diverge from native expectations; spec defines approved contract (including intentional real-size improvement and conservative truncation warning) — tests/evidence must cover it.
- **`fs.list` cap/overflow ambiguity:** 64-entry cap does not distinguish exact-full vs truncated; warning on `count == 64` may be conservative false-positive (accepted). Additive overflow-detection deferred.
- **Implementation-time API gap:** if `fs.list` proves insufficient, work must pause for re-clarification before deviating from spec.
- **Resource-constrained target:** avoid unnecessary allocation/churn in scripts; keep implementation simple and deterministic.

## Progress / evidence
- 2026-11-15–2026-11-17 (clarify-feature): M5 specified and iteratively clarified in `docs/plan/needs-clarification/r22-command-script-migrations-open.md` (flags/format/real sizes/sorting/root/errors/truncation/validation strategy). Spec explicitly approved final 2026-11-15 (no corrections 2026-11-17). Unblock gate and conditional implementation/PR-flow pre-approval recorded 2026-11-17.
- 2026-11-17 (clarify-feature, user decision): accepted existing M4 on-device spot-check record as satisfying gate ("Accept evidence, move M5 to ready now") and approved move to ready. Created this ready item (Ready contract satisfied), moved clarification tracker to `docs/plan/done/r22-command-script-migrations-open.md`. No implementation or validation run.
- **Implementation/validation:** pending.