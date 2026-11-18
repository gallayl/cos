# R20b: Stderr/FD redirection hardened evidence + logging capture playbook

- **ID:** r20-stderr-fd-evidence-logging-playbook
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item b: stderr/FD HW smoke/remaining semantics); docs/reports/2026-11-10-r20-stderr-redirection-parsing-design.md; docs/reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md; docs/plan/consolidated-improvement-plan.md (R20)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Complete validation and documentation for best-effort stderr/FD redirection (`2>`, `2>>`, `&>`, `&>>`): finish HW smoke/evidence, document precisely what is captured (stdio/fd streams) versus what is not (ESP-IDF ESP_LOG* routing/capture caveats), and provide a small tested playbook (commands/scripts) for capturing command/script output to files for on-device debugging.

## Scope
- Define required HW smoke matrix and acceptance evidence for current runtime behavior.
- Document capture semantics and limitations (including ESP_LOG routing) in user-facing/docs form as appropriate.
- Provide a minimal, tested capture playbook (examples) and note any shell/script constraints.

## Non-goals
- ESP log-capture/vprint hook implementation or advanced logging subsystem redesign (unless separately scoped and approved).
- Pipes runtime changes beyond referencing existing behavior.
- Unrelated shell semantics changes.

## Acceptance criteria
- [ ] HW smoke checklist executed on target hardware with recorded results (or explicit blockers recorded per AGENTS.md).
- [ ] Documentation clearly states what is redirected/captured and what is not (with examples).
- [ ] A tested playbook (at least one end-to-end example) exists for capturing stdout/stderr to files in supported cases.

## Validation plan
- Automated: make format-check, make test; relevant host tests for parser/dispatch redirection paths remain green; build/lint as needed if code touched.
- Hardware/manual: required on CYD (or stated target) for stdio redirection behavior to /flash and/or /sdcard; record commands, files produced, and any ESP_LOG observations.

## Expected areas/files
- components/shell/src/shell_redirection.{h,c}, components/shell/src/shell_script.c (if fixes needed), test/test_shell_script.c, docs/reports/*r20*stderr*, README or docs as appropriate.

## Dependencies / prerequisites
- R20 umbrella (docs/plan/needs-clarification/r20-deferred-backlog.md).
- Existing stderr/FD best-effort runtime implementation and design docs listed in Source/links.

## Risks / constraints
- ESP-IDF logging routing differs by component/config; cannot promise full ESP_LOG capture without additional work.
- VFS/path/permissions and storage availability affect test results.

## Open questions
- [ ] What is the minimal required HW smoke set (paths: /flash, /sdcard, both?) and which commands/scripts are canonical?
- [ ] Should documentation live in docs/reports only or also in user-facing README/docs?
- [ ] Any cases where combined redirection (&>) behavior is surprising and must be called out explicitly?

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 5).
