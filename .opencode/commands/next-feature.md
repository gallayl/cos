---
description: Implement the next ready/planned docs feature (composed workflow)
agent: build
---

Follow the `implement-next-feature` skill (repo-specific, composed) and root `AGENTS.md`.

Target/request: $ARGUMENTS

- Run the full composed flow: `ensure-green-baseline` → `pick-next-work-item` → (gate per Ready contract/pre-approval rules) → `prepare-working-branch` → `track-doc-progress` → implement/test/validate (`implementation-workflow`, `testing`/`validate-change`, `add-device-script-example` as applicable) → independent `reviewer`/`code-review` **review-fix loop** for substantive changes (repeat until no valid/actionable findings remain or blocked) → `deliver-pr` → **immediately** `babysit-pr` in the same run until merge-ready or hard-blocked → finalize doc state → report final state.
- Follow `global-gates`, root `AGENTS.md`, and skill-specific gates (including ready-preapproved vs `"go"` rules); merge remains human. Never claim unrun checks.
- Do not stop at PR creation; do not treat PR creation as completion. Report actions, commits/PR links, exact checks run and outcomes, review findings/resolution, and remaining blockers/human actions.
