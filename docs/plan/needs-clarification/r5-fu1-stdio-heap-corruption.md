# R5-FU1 — Stdio/heap corruption investigation (`LoadStoreError` in logging path)

- **ID:** r5-fu1-stdio-heap-corruption
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R5 follow-ups, critical), [`reports/2026-11-08-r5-hardware-results-partial.md`](../../reports/2026-11-08-r5-hardware-results-partial.md), [`history/2026-11-02-memory-consumption-improvements.md`](../../history/2026-11-02-memory-consumption-improvements.md)
- **Approval:** pending (no implementation approval recorded; clarification decisions recorded 2026-11-20)

## Summary
Critical follow-up: investigate `LoadStoreError` at `0x40000000` in stdio flush during R5 GC/checkpoint logging (`collect_berry_garbage` → `esp_log` → `vfprintf` → `stdout_write_hook` in `text_console.cpp`) observed under low-heap/fragmentation conditions (not reproduced in latest captured sample per report, but root cause unresolved).

## Decisions (recorded 2026-11-20, clarify-feature)
- **Reproducibility (user):** No reliable repro now — **explicit deferral** recorded. Keep needs-clarification with unblock conditions documented (not blocked by an unanswered user question).
- **Investigation/fix scope when worked (user):** **Focused instrumentation + root-cause investigation** (Recommended). Instrument/diagnose logging and stdout hook path under pressure: review `text_console` stdout hook ownership/lifetime, add targeted diagnostics, and use heap poisoning/stack checks or equivalent stress tools as feasible to identify root cause before/alongside fix.
- **Closure acceptance criteria (user):** **RCA fixed/mitigated with evidence in defined stress scenarios** (Recommended). Root cause identified and fixed or robustly mitigated, with recorded evidence of no crash in defined stress/repro scenarios (including low-heap/fragmentation + R5 GC/checkpoint logging paths as applicable). Document any residual risk explicitly.

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current blocker (external):** no reliable reproduction case and target environment/firmware state available now.
- **Unblock when:** a reliable repro or actionable stress-repro context is available (target device/board, firmware image/commit/build, steps to trigger low-heap/fragmentation + R5 GC/checkpoint logging, captured logs/backtrace/register state as applicable). Once available, proceed with the recorded focused instrumentation + RCA scope and validate against the recorded closure criteria; promote to ready only when fully specified/approved and implementable per Ready contract.

## Validation plan
- Automated: `make format-check`, `make test`; firmware build/lint as applicable for any code changes.
- Hardware/manual: likely required to reproduce and validate under fragmentation/low-heap conditions and to collect evidence for closure scenarios.

## Progress / evidence
- 2026-11-15: created (critical follow-up; scoping/repro needed).
- 2026-11-20 (clarify-feature, user): recorded explicit deferral (no reliable repro now) with unblock conditions, investigation scope (focused instrumentation + RCA), and closure criteria (RCA fixed/mitigated with evidence in defined stress scenarios). Open questions cleared; item remains needs-clarification due to external repro blocker (not unanswered user questions).
