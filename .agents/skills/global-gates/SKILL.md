---
name: global-gates
description: Canonical shared workflow gates for implementation/PR/docs flows. Compose/reference this instead of restating common gates (approval/ready, green baseline, safe Git, dependency discipline, evidence-based validation/reporting, human merge/hardware limits).
---

# Global gates (shared)

Follow these gates whenever a workflow composes branch/PR/CI/delivery work. Item/skill-specific rules (e.g. Ready contract fields, queue transitions) remain in their owning skills/docs.

## Gates

- **Approval / Git writes:** Require explicit user approval (`"go"`) before implementation or Git write operations when the workflow/item requires it. Items in `docs/plan/ready/` that satisfy the Ready contract are pre-approved for implementation/PR/babysit-to-green flow unless stated otherwise; **merge remains a human/maintainer action**. Do not invent approvals.
- **Green baseline:** Start unrelated feature work only from a healthy baseline. If baseline is red, restore it on a **separate baseline-fix branch/PR** and pause for merge before unrelated feature work; then resume from the healthy base.
- **Safe Git:** No force-push, no history rewrites, no destructive commands, no branch deletion/PR merge by the agent, and no discarding/reverting unrelated user changes. Prefer merging the base branch to sync; resolve conflicts carefully and re-validate.
- **Dependencies:** Never auto-upgrade dependencies unless explicitly in-scope/requested. Use project-pinned mechanisms; routine pinned installs needed to run checks are fine when required and explained.
- **Evidence-based validation/reporting:** Run relevant checks proactively per root `AGENTS.md` and `validate-change`. **Never claim a check passed unless it was executed.** Report exact commands and pass/fail outcomes; for unavailable checks give actionable remediation (what/versions/where/env vars, exact rerun command). Do not rely on CI. State remaining hardware/manual validation when required.
