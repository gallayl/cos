---
name: testing
description: Primary entry procedure for evidence-based testing and verification. Use to plan and run focused checks, type/lint/build/tests by scope, and report exact executed results and blockers (composes validate-change).
---

# Testing (primary)

Primary, portable workflow for verifying changes with executed evidence.

## Workflow

1. **Understand scope and risk.** Identify affected areas, behaviors, and regressions/edge cases that matter.
2. **Follow existing conventions.** Use the repo's established test frameworks/locations and run checks via Makefile targets / frontend `yarn` scripts per root `AGENTS.md`.
3. **Run focused checks first.** Start with the smallest relevant tests/checks for the changed behavior, then broaden (type checking, linting, builds, integration tests) as scope requires.
4. **Compose `validate-change`** for scope-appropriate commands, execution, and reporting (`.agents/skills/validate-change/SKILL.md`).
5. **Assess coverage.** Add or update unit/integration tests when behavior changes or regressions are plausible; flag gaps that remain.
6. **Report evidence.** State exactly what ran and outcomes; **never claim a check passed unless it was executed**. For environment/hardware limits, report actionable blockers and what remains.

## Layering

- This is a **primary entry procedure**. It composes the granular `validate-change` skill to avoid duplicating command matrices/reporting rules.
- Keep repo policy in `AGENTS.md` authoritative; do not invent alternate check commands when standard targets exist.
