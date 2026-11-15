---
description: Run the generic implementation workflow
agent: build
---

Follow the `implementation-workflow` skill (primary, generic) and root `AGENTS.md`.

Task: $ARGUMENTS

- Understand requirements/acceptance criteria, inspect relevant code/docs, and compose granular skills as needed (`ensure-green-baseline`, `prepare-working-branch`, `testing`/`validate-change`, `track-doc-progress`, `deliver-pr`, `babysit-pr`, `add-device-script-example`, `code-review`).
- Preserve required gates: explicit approval ("go") before implementation/Git writes when the workflow requires it, green baseline (separate baseline-fix branch/PR merged before unrelated feature work when applicable), safe Git (no force-push/history rewrites, never merge PRs), and evidence-based validation/reporting (never claim unrun checks).
- For the repo-specific "implement next feature from docs" flow, use `pick-next-work-item` and the `implement-next-feature` composed workflow instead of treating this command as docs-specific.
- Report scope, actions, exact checks run and outcomes, and any blockers/remaining manual or hardware validation.
