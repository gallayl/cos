---
description: Run the generic implementation workflow
agent: build
---

Follow the `implementation-workflow` skill (primary, generic) and root `AGENTS.md`.

Task: $ARGUMENTS

- Understand requirements/acceptance criteria, inspect relevant code/docs, and compose granular skills as needed (`ensure-green-baseline`, `prepare-working-branch`, `testing`/`validate-change`, `track-doc-progress`, `deliver-pr`, `babysit-pr`, `add-device-script-example`, `code-review`). For substantive changes, use the read-only `reviewer` subagent for independent `code-review` and run a **review-fix loop** (resolve actionable findings, re-validate, re-review) until no valid/actionable findings remain or blocked; a background `test-runner` subagent is not required (CI runs tests), but follow evidence-based validation/reporting for checks you run.
- Follow `global-gates` and root `AGENTS.md` (approval/ready, green baseline, safe Git, dependency discipline, evidence-based validation/reporting; merge remains human). For docs-next work, apply ready-preapproved vs `"go"` rules per `implement-next-feature`/`pick-next-work-item`.
- For the repo-specific "implement next feature from docs" flow, use `pick-next-work-item` and the `implement-next-feature` composed workflow instead of treating this command as docs-specific.
- Report scope, actions, exact checks run and outcomes, and any blockers/remaining manual or hardware validation.
