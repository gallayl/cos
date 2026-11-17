---
name: code-review
description: Primary entry procedure for independent, evidence-based code review. Use to identify real defects, risks, missing validation/tests, and actionable fixes with severity-focused findings.
---

# Code review (primary)

Primary workflow for reviewing a named change (uncommitted/staged changes, branch, PR, or files). If nothing is named, review uncommitted changes, or the last commit if there are none. Reviewer/subagent use is **read-only** (do not modify implementation files).

## Workflow

1. **Understand intent, scope, and load.** Read the change and the code it touches: callers of every changed function, functions it calls, tests, and README/docs when they define expected load/usage (one user or many users/processes). A change can break code it does not touch.
2. **Review for actual issues (not style/taste, not vague worries), in priority order:**
   - **Bugs/correctness/regressions:** wrong result, crash, missed edge case, broken caller/contract, fix applied in one caller while shared/root cause remains.
   - **Risk:** security holes, data loss/corruption, unsafe error handling, concurrency hazards (e.g. check-then-write races), resource/heap/flash impact (ESP32 constraints), compatibility issues.
   - **Scale/performance:** only-grow memory/state, repeated work per process, query-per-item, O(n^2) on realistically large input (big = problem, small = suggestion) given assumed load.
   - **Missing validation/tests:** risky logic without a test/check; missing required docs/invariants (e.g. SDK↔Berry parity, Doxygen/format/DoD evidence) when applicable.
   - **Lean opportunities:** dead code, duplicate/near-copy to merge, helper/pattern the repo already has, stdlib/platform/native where appropriate, abstractions with one implementation, or a function doing several unrelated jobs to split (never split by line count) — propose only when concrete and beneficial.
3. **Confirm and ground findings.** Re-read the relevant lines. Every finding needs a concrete case/impact; distinguish confirmed issues from risks/assumptions. A shortcut explicitly marked with a `shortcut:` (or legacy `ponytail:`) comment that names its limit is a decision, not a finding.
4. **Compose supporting procedures when useful:** use `validate-change`/`testing` to reason about or run required checks (if authorized) and reference `add-device-script-example` when script-facing APIs change. Do not claim checks passed unless executed.
5. **Report actionable, severity-classified findings** (e.g. Must fix / Should fix / Nice to have, or critical/high/medium/low) with clear `file:line` references. For each finding state **what is wrong**, **why it matters/impact** (and concrete case), and a **concrete fix/verification step** (prefer the smallest fix; prefer deleting code over adding layers). Note assumptions, assumed load, and checks not run.
6. **Fix/re-review loop (when fixes are in scope):** after actionable findings are addressed, re-review the updated change; repeat until no valid/actionable findings remain or a blocker requires a decision. Do not dismiss actionable feedback.

## Output

- Start with **`What this change does:`** (2–3 sentences) and the **load assumed**.
- Then numbered findings grouped by severity; each with a bold title including `file:line` and concise bullets/details for problem, concrete case/impact, fix, and if-skipped risk (adapt detail to severity).
- End with **`Verdict:`**, **`Lean: -N lines possible.`** when lean findings exist, and **`Not checked:`** for anything important you could not verify. If nothing found: **`Looks good. Ship.`**
- Base conclusions on evidence (code/diffs/executed results). Keep language clear; explain technical terms when needed.
