---
name: code-review
description: Primary entry procedure for independent, evidence-based code review. Use to identify real defects, risks, missing validation/tests, and actionable fixes with severity-focused findings.
---

# Code review (primary)

Primary, portable workflow for reviewing changes (diff/PR/files) against correctness and repo standards.

## Workflow

1. **Understand intent and scope.** Read the change, related code paths, and applicable instructions (`AGENTS.md`, nested `AGENTS.md`, relevant skills/docs).
2. **Review for actual issues (not style nitpicks):** correctness, regressions, security, data loss, resource/heap/flash impact (ESP32 constraints), error handling, backwards compatibility, concurrency/edge cases, and missing validation/tests.
3. **Check repo invariants and DoD alignment:** conventions, API/SDK↔binding parity when applicable, Doxygen/format expectations, and whether required checks were (or must be) run with evidence.
4. **Compose supporting procedures when useful:** use `validate-change` to run or reason about required checks, and reference `add-device-script-example` when script-facing APIs change.
5. **Classify findings by severity** (e.g. critical/high/medium/low) with clear file/line references and impact.
6. **Provide actionable recommendations.** For each finding state what is wrong, why it matters, and a concrete fix/verification step. Note any assumptions or checks not executed.

## Notes

- Prefer evidence over speculation; distinguish confirmed issues from potential risks.
- Reviewer role, when used as an agent/subagent, should remain read-only and must not modify implementation files.
