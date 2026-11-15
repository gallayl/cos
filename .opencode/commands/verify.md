---
description: Run project verification for the current scope and report evidence
agent: build
---

Follow the `testing` and `validate-change` skills and root `AGENTS.md`.

Scope/context: $ARGUMENTS

- Determine the affected scope and run relevant checks proactively using standard Makefile targets and frontend `yarn` scripts.
- Report exact commands and pass/fail outcomes. Never claim a check passed unless executed; for unavailable checks provide actionable remediation (what/versions/where/env vars, exact rerun command) and state what remains (including hardware validation if applicable).
- Fix failures when in scope and re-run until passing, or clearly report blockers.
