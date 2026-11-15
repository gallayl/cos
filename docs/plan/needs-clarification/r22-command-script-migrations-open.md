# R22 — Berry command-script migrations (open questions / remaining milestones)

- **ID:** r22-command-script-migrations-open
- **Status:** needs-clarification (blocked: M4 must land before M5 moves to ready)
- **Created:** 2026-11-15
- **Updated:** 2026-11-15 (clarification rounds 1-4; M4 approved/moved to ready; M5 spec approved but blocked)
- **Source/links:** [`plan/2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (M4 clarified/ready 2026-11-15, M5 spec approved 2026-11-15 blocked by M4 dependency), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22 in progress), **M4 ready item:** [`plan/ready/r22-m4-time-read-only.md`](../ready/r22-m4-time-read-only.md)
- **Approval:** M4 explicitly approved and pre-approved for implementation/PR flow on 2026-11-15 (user chat; merge remains human). **M5 spec explicitly approved in chat on 2026-11-15** (proposed draft in this doc accepted as written); **not ready/moved** because dependency blocker remains (M4 implementation/validation must land first, per user decision). Pre-approval for M5 implementation/PR flow is **not** recorded until moved to ready.

## Summary
R22 has delivered M1/M2a/M2b/M2c and M3 implementation exists (hardware validation tracked separately). M4 (`time` read-only view) has been clarified/spec-drafted for promotion to `ready` (pending explicit approval). M5 (`ls`/`dir`) remains **needs-clarification** due to parity/contract risk and is deferred until after M4 lands.

## Scope
- Clarify and, once specified/approved, implement remaining R22 migrations and resolve open questions per the R22 plan (each migration API-gated, contract-preserving or explicitly approved, with tests/evidence).
- **This clarification cycle target:** specify **M4 only** as ready (separate item). Record global decisions applicable to R22 and keep M5-specific questions/blockers open.

## Non-goals
- Migrating commands listed as "keep native for now" unless separately justified/approved.
- Using `action()` + console-output parsing for structured results.
- Making M5 ready in this cycle (deferred; see open questions).
- Moving clock synchronization into scripts (M4 sync control remains native).

## Decisions / resolved questions (2026-11-15, user answers)

- [x] **Which milestone(s) to specify ready now?** **M4 only.** M4 to be promoted as a separate ready item (pending explicit approval). M5 remains in `needs-clarification`.
- [x] **Default discoverability / bootstrap:** Add **`/flash/scripts` as a built-in default search directory** in the script resolver so stock migrated command scripts work out-of-box even with an empty `/flash/.cos_path`. Resolver behavior to be documented (implicit default, searched per resolver rules; user `path` config still applies). Stock image/script placement remains under `/flash/scripts`.
- [x] **`memory` alias/script:** **Keep both `mem` and `memory` as separate scripts** (`mem.be`/`memory.be`, and TS sources where maintained), matching current migrated state. Intentional, no consolidation required.
- [x] **M4 Berry API (exact shape):** Add **`system.time_info()`** (Berry module `system`, TS SDK `@cos/system` / global `system`) returning a structured map (no console scraping):
  - Success: `{ ok: true, synced: boolean, epoch: number, utc: string, local: string }`
    - `synced`: `time_sync_is_synced()`
    - `epoch`: `time_sync_get_epoch()` value
    - `utc`: formatted via `time_sync_get_utc(...)` (ISO 8601 UTC string, e.g. `2026-02-18T14:30:00Z`)
    - `local`: formatted via `time_sync_get_local(...)` (ISO 8601 with offset, e.g. `2026-02-18T15:30:00+01:00`)
  - Failure (unexpected formatting/error case): `{ ok: false, error: string }` where `error` is an ESP-IDF error name (e.g. `ESP_ERR_INVALID_SIZE`) or equivalent stable error identifier. Normal operation expected to succeed with adequate internal buffers.
- [x] **M4 command contract/parity:** **Strict parity** with current native `time` behavior (`components/time_sync/src/time_sync_cmd.c`):
  - `time` / no recognized subcommand (including unknown args, current fallthrough) prints full status: `Local:  <local>`, `UTC:    <utc>`, `Synced: yes|no` (same labels/spacing/order) and returns 0 on success.
  - `time utc` prints `<utc>` and returns 0 on success.
  - `time epoch` prints `<epoch>` (decimal integer form, e.g. `printf("%lld\n", ...)` equivalent) and returns 0 on success.
  - Formatting/API failure prints `time: format error (<error>)` and returns 1.
  - Script must not move/alter clock synchronization behavior (sync remains native).
- [x] **M4 validation:** **Host/automated + on-device spot-check.** Automated: `make format-check`, `make test`, and scope-appropriate checks per `AGENTS.md` (firmware build/lint/docs-check as applicable; frontend/SDK checks if TS/SDK declarations/manifest/bindings changed). Host/unit tests for arg handling/script-API wiring/formatting logic where feasible. Hardware/manual on device: verify `time`, `time utc`, `time epoch` in recorded sync states (synced/not-synced or as available), confirm output/status parity and that sync remains native/unaffected. Full NTP reproducibility not required if behavior/evidence is clearly recorded. QEMU not required for NTP.
- [x] **M5 staging/scope/priority:** **Defer M5; specify after M4 lands.** Keep M5 in `needs-clarification`. Before making M5 ready: confirm `fs.list` contract sufficiency for parity (sizes, dirs, root `/` behavior, 64-entry/`VFS_ENTRIES_MAX` handling, error mapping), decide strict parity vs explicitly approved formatting fixes (e.g. always show real file sizes vs current readdir fast-path `0` size quirk in native `ls`), and define supported subset explicitly (currently `[path]` only, no flags). `dir` remains alias of `ls` unless explicitly changed.
- [x] **M5 (2026-11-15 clarify-feature, round 1): clarify focus = M5 spec.** File sizes: **show real file sizes** (approved improvement over native readdir fast-path `0` quirk; must be documented as intentional diff). Initial scope preference: **add common flags now** (e.g. `-l`/`-a`) — requires detailed flag/output/arg-parsing spec. Large dirs/errors: **best-effort + explicit truncation note** (show up to cap, print clear truncation warning/message; define stable error strings/exit codes; deterministic ordering if feasible) — exact messages/behavior still to be specified.
- [x] **Integration-test/QEMU strategy:** **Host tests + targeted QEMU where feasible.** Prefer host/unit tests for arg parsing, script-API wiring, and pure formatting logic; add/extend QEMU/integration tests when they can exercise VFS/listing or scripted command behavior deterministically. Do not require QEMU coverage for NTP/time sync; rely on on-device validation for time/network-dependent cases. Document gaps/limitations per milestone.

## Acceptance criteria
- [x] Open questions relevant to **M4 ready specification** resolved and recorded (with approved decisions). M4 moved to ready.
- [x] **M4** specified as ready and moved to `docs/plan/ready/r22-m4-time-read-only.md` after explicit approval (2026-11-15, pre-approved for implementation/PR flow; merge remains human).
- [x] **M5** fully specified and **explicitly approved** (2026-11-15, proposed draft accepted). Ready-contract fields documented in this item; **not moved to ready** due to dependency blocker (M4 must land first).
- [ ] M4 implemented with structured API, script(s), docs, tests, and validation evidence per R22 checklist; native `time` registration removed only when validated. (future implementation)
- [ ] M5 moved to `docs/plan/ready/` (with pre-approval recorded) once M4 implementation/validation evidence lands and no new gaps exist; then implemented/validated per R22 checklist. (future)
- [ ] `docs/plan/2026-11-12-berry-command-script-migrations.md` and consolidated plan R22 updated with status/evidence as work progresses.

## Validation plan
- **M4 (spec):** decisions recorded above. **M4 implementation (when approved/implemented):** `make format-check`, `make test`, scope checks per `AGENTS.md` (firmware build/lint, docs-check, frontend/SDK/IDE binding checks if affected), host/unit tests where feasible, targeted QEMU where deterministic/feasible, and **on-device spot-check** for `time`/`time utc`/`time epoch` + sync-native behavior with evidence recorded. No QEMU NTP requirement.
- **M5:** to be specified when clarified (likely host/QEMU for VFS parity where feasible + on-device flash/SD spot-check).

## Expected areas/files
- **M4:** `components/time_sync/` (keep sync/native init/behavior; native `time` cmd removed only after migration validated), `components/berry_lang/` (`system.time_info` binding), `components/shell/` (no `time` native registration after migration), `frontend/src/cos-sdk/system.d.ts` (and any generated/manifest bindings), `storage/ts/scripts/time.ts` and `storage/flash_data/scripts/time.be`, `test/`, `integration_test/` (targeted if feasible), `docs/reports/` for hardware evidence, README/help/docs updates.
- **Discovery/default:** `components/shell/src/shell_script.c` (or resolver path logic) to include implicit `/flash/scripts` default; docs/tests for path behavior.
- **M5 (later):** `components/filesystem/`, Berry `fs.list` contract/bindings if gaps found, `components/shell/` native `ls`/`dir` removal, scripts, tests, docs.

## Dependencies / prerequisites
- **M4:** existing `time_sync_*` APIs are sufficient for read-only view; `system.time_info()` binding + SDK declarations + stock script + default discoverability behavior implemented/tested.
- **M5 (deferred):** `fs.list` parity/limitations resolved (64-entry cap, metadata, root semantics, error mapping) and explicit parity decisions recorded.

## Risks / constraints
- M4: SNTP/network-dependent sync state makes full automated reproducibility hard; strict parity requires matching fallthrough/spacing/error behavior exactly. Berry number representation of epoch should follow existing runtime conventions (document if relevant).
- M5: high parity/UX risk (flags/format/errors/large dirs/memory, current native readdir fast-path size reporting behavior); may need staged/approved diffs.
- Default `/flash/scripts` search: must not break explicit path/user-configured precedence expectations; document and test precedence (CWD, configured path, implicit default) clearly.

## Proposed M5 spec (draft, 2026-11-15, pending explicit approval & M4 dependency)

*Decisions incorporated:* real file sizes (intentional diff from native readdir fast-path 0-size quirk), common flags now, best-effort truncation note, name-sorted deterministic output, defined root/errors baseline. **Not ready to move yet:** dependency/prerequisite — make M5 ready only after M4 implementation/validation lands (recommended, previously agreed). `dir` implemented as alias script/identical behavior to `ls` unless corrected.

- **Commands/aliases:** `ls`, `dir` (identical args/output/exit behavior).
- **Usage:** `ls [-a] [-l] [path]` and `dir [-a] [-l] [path]`. Flags may appear before path as `-a`, `-l`, `-la`, or `-al`. `path` defaults to CWD when omitted. At most one path argument. Unknown flags, extra args, or invalid flag forms print `Usage: ls [-a] [-l] [path]` (or `dir ...` matching invoked name) and exit 1.
- **`-a`:** include entries whose names start with `.` if returned by listing; `.` and `..` are never shown (VFS omits them). (Note: current VFS/fs.list may not return dotfiles; behavior is specified for forward-compat/correctness.)
- **`-l` (simple long format):** one row per entry: `<type> <size> <mtime> <name><suffix>` where `<type>` is `d` for directory else `-`, `<size>` is decimal bytes (real size from fs.list; directories may show 0), `<mtime>` is decimal `mtime` seconds (fs.list value; 0 if unavailable), `<name>` is entry name and `<suffix>` is `/` for directories else empty. Columns separated by single spaces. Summary line preserved after entries: `%u file(s), %u dir(s), %u bytes` (bytes summed from file sizes shown); empty dir prints `(empty)`.
- **Default (no `-l`):** preserve native style but with real sizes: directories `  <DIR>  <name>/`, files `  %5u  <name>` using actual file size (right-aligned in >=5 chars as today). Summary/empty behavior as current.
- **Ordering:** entries sorted by name (deterministic, byte/string compare) before printing for both plain and `-l` views. Root `/` synthetic entries sorted by name (`flash`, `sdcard` when present).
- **Root `/`:** list available filesystems as today (`flash/`, `sdcard/` when mounted) adapted to selected format and real/known metadata (size/mtime 0 if not applicable), with matching summary counts/bytes.
- **Errors/exit:** invalid/unresolvable path -> `ls: invalid path` and exit 1; cannot open/list/not-a-directory/access error -> `ls: cannot open '<path>'` (or equivalent stable message matching native intent; exact parity to be validated) and exit 1. Usage errors exit 1. Success exits 0.
- **Truncation (64-entry cap, `VFS_ENTRIES_MAX`):** if `fs.list` returns `count == 64`, show listed entries (sorted subset as returned/processed) and print warning `ls: warning: listing may be truncated at 64 entries` after output/summary; **exit 0**. (fs.list does not indicate overflow beyond cap; warning is conservative. Additive API for accurate overflow detection deferred unless required.)
- **API sufficiency:** current `fs.list` (`name,size,is_dir,mtime,ctime`, max 64, structured ok/error) is expected sufficient for this spec; confirm during implementation (no additive API changes planned now).
- **Validation (when implemented):** host/unit tests for arg parsing/formatting/sorting/truncation/error cases; targeted QEMU/VFS/script-dispatch tests where deterministic/feasible; on-device spot-check on `/flash` and `/sdcard` (if available) including flags and truncation behavior as reproducible. `make format-check`, `make test`, scope checks per `AGENTS.md`.

## Open questions / blockers
- [ ] **M5 dependency blocker (not Ready):** M5 spec approved, but item **must remain in needs-clarification until M4 implementation/validation lands** (user decision 2026-11-15). When M4 lands with evidence and no new M5 gaps are found, confirm and move M5 to `docs/plan/ready/` with pre-approval recorded.
- [ ] **M5 (deferred):** is current `fs.list` API contract sufficient as-is for this spec in practice, or are additive fields/behavior changes needed (discover during implementation; if needed, re-clarify)?
- [ ] **M3 (separate item):** hardware validation evidence/blockers tracked in [`r22-m3-light-hardware-validation.md`](./r22-m3-light-hardware-validation.md) (not answered in this session). No M5 spec questions remain open.

## Progress / evidence
- 2026-11-15: created to capture R22 open questions and remaining M4/M5 planning needs.
- 2026-11-15 (clarification): user decisions recorded — M4 only to specify ready now; add `/flash/scripts` to built-in default resolver search; keep `mem` and `memory` as separate scripts; M4 API `system.time_info()` structured map (synced/epoch/utc/local, ok/error); strict parity for `time`/`time utc`/`time epoch` including unknown-arg fallthrough and error/status behavior; sync remains native; validation = host/automated + on-device spot-check (no QEMU NTP); M5 deferred after M4 with parity/`fs.list` decisions pending; test strategy = host tests + targeted QEMU where feasible. Source/consolidated plan updated.
- 2026-11-15 (approval): user explicitly approved M4 spec and pre-approved implementation/PR flow (merge remains human). **M4 moved to ready:** [`../ready/r22-m4-time-read-only.md`](../ready/r22-m4-time-read-only.md). M5 remains needs-clarification.
- 2026-11-15 (re-review): M4 ready item re-reviewed; Ready contract satisfied and remains `ready` (approved). No additional spec gaps/questions identified.
- 2026-11-15 (clarify-feature session, round 1): re-reviewed R22 state for clarification. Located split items: M4 ready (`docs/plan/ready/r22-m4-time-read-only.md`), M3 implementation present but hardware validation blocked/open (`docs/plan/needs-clarification/r22-m3-light-hardware-validation.md`), remaining open spec tracked here (primarily **M5 `ls`/`dir`**, deferred after M4). User chose **clarify M5 spec** now: **real file sizes** (intentional diff), **add common flags** (`-l`/`-a` indicated), **best-effort + explicit truncation note** for capped listings. Recorded decisions; asked round 2 follow-up questions for exact flag set/behavior/output/errors/truncation text/API sufficiency. M5 remains needs-clarification.
- 2026-11-15 (clarify-feature session, round 2): user confirmed `-a`/`-l`/`-la`/`-al`, simple long rows+summary, warn if count==64 (exit 0), name-sorted with defined root/errors. Drafted concrete **Proposed M5 spec** in this doc (usage/parsing, formats, ordering, truncation message, errors, validation, API note). Awaiting explicit approval/corrections and handling of M4 dependency blocker before any Ready transition.
- 2026-11-15 (clarify-feature session, round 3, explicit approval): user **explicitly approved M5 spec as drafted** and chose to **keep item in needs-clarification until M4 implementation/validation lands** (dependency blocker; prior timing decision honored). No spec corrections requested. M5 remains needs-clarification (approved-but-blocked); not moved to `docs/plan/ready/`, no M5 implementation pre-approval recorded. R22/M5 tracking docs updated for consistency.
