# R20b: Stderr/FD redirection hardened evidence + logging capture playbook

- **ID:** r20-stderr-fd-evidence-logging-playbook
- **Status:** done (delivered; on-device CYD HW smoke evidence pending human validation)
- **Created:** 2026-11-18
- **Delivered:** 2026-11-20
- **PR:** [#111](https://github.com/gallayl/cos/pull/111) (`feat/r20b-stderr-fd-evidence-playbook`, docs-only)
- **CI (required checks green):** [44767477786](https://github.com/gallayl/cos/actions/runs/44767477786) (success, head `4299d4670c11bc5fff80f298fa47ac41f6772841`)
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item b); [`../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md`](../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md); [`../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20)
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
- [x] HW smoke checklist executed on CYD for both `/flash` and `/sdcard` (or blockers documented per `AGENTS.md`) with recorded results: commands, produced files, and `ESP_LOG*` observations. — **Canonical matrix/checklist, commands, expected file checks, and `ESP_LOG*` observations to record provided in [`../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md`](../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md); on-device execution not performed in this environment. Concrete blockers documented (no CYD device/serial console access; SD card presence/mount state unknown) with remediation/steps for human evidence collection.**
- [x] Documentation updated: report/playbook in `docs/reports/*r20*stderr*` (and design notes adjusted if needed) **and** a short user-facing note (README or docs page) stating what is captured vs not, with examples/link. — **Playbook/report added; [`../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md`](../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md) cross-linked; short user-facing note added in [`../../../../README.md`](../../../../README.md).**
- [x] One tested end-to-end playbook example exists (shell session) covering `2>`, `2>>`, `&>` (and append/truncate sanity as appropriate) with verification steps. — **Provided in playbook/report; syntax/structure verified against in-repo shell redirection implementation/contracts and existing `>`/`>>` hardware evidence; on-device execution blocked/pending human validation with expected outcomes documented.**
- [x] Callouts for `&>`/`&>>` include shared-file behavior, stdio/fd-only scope, `ESP_LOG*` caveat, and reaffirmation of existing limits (multiple redirections rejected, path/dir/open errors unchanged in spirit). — **Included in playbook/report and summarized in README note.**

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

## Delivered artifacts (this change)
- Playbook/evidence/matrix: [`../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md`](../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md)
- User-facing note: [`../../../../README.md`](../../../../README.md) (Console section, output redirection summary + link)
- Design note cross-link: [`../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md`](../../reports/2026-11-10-r20-stderr-redirection-parsing-design.md)
- No shell/parser/dispatch/runtime or test code changes required by evidence/scope.

## Validation / evidence
- **Local (executed):** `make format-check` — pass (exit 0). `make test` — pass (CTest 31/31, Python `scripts` unittest 3/3 OK, exit 0). `make docs-check` — pass (exit 0).
- **Not run locally:** `make build` / `make lint` not required (no firmware/C/C++ code changes) and ESP-IDF v6.1/clang-tidy environment not exercised for this docs-only scope; CI-covered checks relied on via PR CI as applicable.
- **CI:** PR [#111](https://github.com/gallayl/cos/pull/111) required checks green, run [44767477786](https://github.com/gallayl/cos/actions/runs/44767477786) (success, head `4299d4670c11bc5fff80f298fa47ac41f6772841`).

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 5).
- 2026-11-18 (clarify-feature): reviewed R20 stderr design/reports. User choices: HW smoke on both /flash and /sdcard with canonical matrix; docs in reports + short user-facing note (README/docs) linking to playbook; explicit &> callouts including ESP_LOG caveat and existing limits; one end-to-end shell session playbook. Open questions resolved.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
- 2026-11-20 (implementation/delivery): added canonical playbook/report and HW smoke matrix with concrete blockers (no CYD device/serial console; SD availability unknown) and "not executed" evidence plus remediation steps/commands for human collection; added end-to-end shell session example (syntax/structure verified against in-repo implementation/contracts and existing `>`/`>>` HW evidence) with `ESP_LOG*`/`&>` callouts and limits/flush-ordering notes; added README user-facing note and design-doc cross-link. Local validation: `make format-check` pass, `make test` pass (31/31 CTest, 3/3 Python), `make docs-check` pass. PR [#111](https://github.com/gallayl/cos/pull/111) opened; CI run [44767477786](https://github.com/gallayl/cos/actions/runs/44767477786) required checks green. Item moved `ready/` → `done/` with evidence recorded.

## Remaining human actions
- **Review/approval and merge (human/maintainer).** Do not merge by automation.
- **Required on-device CYD hardware smoke validation (merge-blocking until evidence recorded or explicitly waived by human):** execute canonical matrix on both `/flash` and `/sdcard` (when mounted/writable) per [`../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md`](../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md); record commands, produced file contents/sizes, and `ESP_LOG*` observations (expected: often not captured), plus device/build/storage details and any deviations. If SD unavailable, document explicitly and complete `/flash` evidence. Append results to the playbook/report (or linked follow-up) and update PR/item status as appropriate.
