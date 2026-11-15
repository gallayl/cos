# R5-FU1 — Stdio/heap corruption investigation (`LoadStoreError` in logging path)

- **ID:** r5-fu1-stdio-heap-corruption
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R5 follow-ups, critical), [`reports/2026-11-08-r5-hardware-results-partial.md`](../../reports/2026-11-08-r5-hardware-results-partial.md), [`history/2026-11-02-memory-consumption-improvements.md`](../../history/2026-11-02-memory-consumption-improvements.md)
- **Approval:** pending

## Summary
Critical follow-up: investigate `LoadStoreError` at `0x40000000` in stdio flush during R5 GC/checkpoint logging (`collect_berry_garbage` → `esp_log` → `vfprintf` → `stdout_write_hook` in `text_console.cpp`) observed under low-heap/fragmentation conditions (not reproduced in latest captured sample per report, but root cause unresolved).

## Open questions
- [ ] Is there a reliable reproduction case and target environment/firmware state to debug now?
- [ ] Scope of fix/investigation (instrumentation, heap poisoning/stack checks, ownership/lifetime in `text_console` stdout hook, logging behavior under pressure)?
- [ ] Acceptance criteria for closure (no crash in defined stress scenarios, root cause identified/fixed or mitigated with evidence)?

## Validation plan
- Automated: `make format-check`, `make test`; firmware build/lint if code changes.
- Hardware/manual: likely required to reproduce and validate under fragmentation/low-heap conditions.

## Progress / evidence
- 2026-11-15: created (critical follow-up; scoping/repro needed).
