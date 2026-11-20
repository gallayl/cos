# R20b: Stderr/FD redirection hardened evidence + logging capture playbook

- **ID:** r20-stderr-fd-evidence-logging-playbook
- **Status:** ready (approved)
- **Created:** 2026-11-18
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item b: stderr/FD HW smoke/remaining semantics); [`../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md`](../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md); [`../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
Complete validation and documentation for best-effort stderr/FD redirection (`2>`, `2>>`, `&>`, `&>>`): finish HW smoke/evidence, document precisely what is captured (stdio/fd streams) versus what is not (ESP-IDF `ESP_LOG*` routing/capture caveats), and provide a small tested playbook (one end-to-end shell session example) for capturing output to files on-device.

## Scope (decisions recorded 2026-11-18)
- **HW smoke matrix (1A, minimal, CYD):** test on **both `/flash` and `/sdcard`** (when available/mounted as applicable) with a canonical, repeatable set: use simple commands that produce normal stdout and/or generate handled errors/messages that go through redirected stdio paths where applicable; explicitly test `2>`, `2>>`, `&>`, and sanity-check append/truncate behavior consistent with existing `>`/`>>` patterns. Record commands run, files produced, and any `ESP_LOG*` observations (expected: may not appear in redirected files).
- **Documentation location (2A):** record evidence in `docs/reports/*r20*stderr*` (and update design notes if needed) **plus** a short user-facing note in README or a docs page summarizing “captured vs not (stdio/fd only; `ESP_LOG*` caveat)” with a link to the report/playbook.
- **Callouts for `&>` / `&>>` (3A):** document explicitly: (a) shared target file for combined redirection (`&>`/`&>>`), (b) redirection applies to stdio/fd producers; **`ESP_LOG*` may bypass or be routed differently** and is not guaranteed in redirected files, (c) existing limits still apply (multiple redirections rejected, path/dir/open errors behave as before per current design), (d) note flush/ordering expectations in practice (best-effort) as observed.
- **Playbook format (4A):** provide **one end-to-end shell session example** (copy-paste commands) demonstrating stdout/stderr/both redirection to files and verification (e.g. `cat`/`ls`), plus concise notes about `ESP_LOG*` caveat.

## Non-goals
- ESP log-capture/vprint hook implementation or advanced logging subsystem redesign (unless separately scoped and approved).
- Pipes runtime changes beyond referencing existing behavior.
- Unrelated shell semantics changes.

## Acceptance criteria
- [ ] HW smoke checklist executed on CYD for both `/flash` and `/sdcard` (or blockers documented per `AGENTS.md`) with recorded results: commands, produced files, and `ESP_LOG*` observations.
- [ ] Documentation updated: report/playbook in `docs/reports/*r20*stderr*` (and design notes adjusted if needed) **and** a short user-facing note (README or docs page) stating what is captured vs not, with examples/link.
- [ ] One tested end-to-end playbook example exists (shell session) covering `2>`, `2>>`, `&>` (and append/truncate sanity as appropriate) with verification steps.
- [ ] Callouts for `&>`/`&>>` include shared-file behavior, stdio/fd-only scope, `ESP_LOG*` caveat, and reaffirmation of existing limits (multiple redirections rejected, path/dir/open errors unchanged in spirit).

## Validation plan
- **Automated:** `make format-check`, `make test`; relevant host tests for parser/dispatch redirection paths remain green; `make build`/`make lint`/`make docs-check` as needed if code/docs touched (per `AGENTS.md`).
- **Hardware/manual (required):** CYD session exercising the matrix on `/flash` and `/sdcard`; record exact commands, file contents checks, and observations (including whether any `ESP_LOG*` output appeared in redirected files). If unavailable, record concrete blockers.

## Expected areas/files
- `components/shell/src/shell_redirection.{h,c}`, `components/shell/src/shell_script.c` (only if fixes required)
- `test/test_shell_script.c` (only if test gaps found)
- `docs/reports/*r20*stderr*` (evidence/playbook), design docs under `docs/reports/2026-11-10-r20-stderr-redirection-parsing-design.md` updates if needed
- User-facing note in `README.md` or a short docs page (location chosen in implementation)

## Dependencies / prerequisites
- R20 umbrella: [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (item b).
- Existing stderr/FD best-effort runtime and prior R20 stderr reports/design docs.

## Risks / constraints
- `ESP_LOG*` routing is environment/config dependent; playbook must state it is not guaranteed to be captured.
- VFS/storage availability (SD card present/mounted) affects HW smoke; document assumptions.
- No expansion into log-capture hooks in this item.

## Open questions
- None (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A). If HW reveals unexpected behavior, record it and update acceptance/evidence rather than expanding scope.

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 5).
- 2026-11-18 (clarify-feature): reviewed R20 stderr design/reports. User choices: HW smoke on both /flash and /sdcard with canonical matrix; docs in reports + short user-facing note (README/docs) linking to playbook; explicit &> callouts including ESP_LOG caveat and existing limits; one end-to-end shell session playbook. Open questions resolved.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
