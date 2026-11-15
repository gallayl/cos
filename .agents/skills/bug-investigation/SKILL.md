---
name: bug-investigation
description: Primary entry procedure for investigating and fixing bugs with root-cause analysis and verification. Use for defects, regressions, flaky behavior, or failing checks that need reproduction and evidence-based resolution.
---

# Bug investigation (primary)

Primary, portable workflow for diagnosing and fixing bugs.

## Workflow

1. **Understand and reproduce.** Capture the reported symptom, expected vs actual behavior, environment, and minimal reproduction. Gather logs/traces/test output and relevant evidence.
2. **Locate the root cause.** Inspect the implicated code paths, recent changes, and callers/data flow. Form hypotheses and validate them against evidence (do not guess).
3. **Propose the smallest appropriate fix.** Address the root cause (not just symptoms), preserve invariants and surrounding behavior, and consider edge cases/regressions.
4. **Implement and add coverage.** Apply a focused fix and add/update tests when feasible to prevent recurrence.
5. **Verify.** Compose `validate-change` (and `testing` as needed) to run relevant checks and confirm the fix; report exact executed commands/outcomes.
6. **Report.** Summarize root cause, fix, evidence, and any remaining risks or environment/hardware limitations.

## Layering

- Primary entry procedure; composes granular skills such as `validate-change` (and may feed into `prepare-working-branch`/`deliver-pr` when part of a larger change workflow) without duplicating their procedures.
