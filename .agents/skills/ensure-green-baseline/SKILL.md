---
name: ensure-green-baseline
description: Granular procedure to verify a healthy baseline (repo state, base branch, CI) and, if red, restore it via a separate baseline-fix branch/PR before unrelated feature work. Use when composing workflows that require a green base.
---

# Ensure green baseline (granular)

A green baseline is required before building unrelated feature work. **Granular skill** composed by primary/repo-specific workflows.

## Workflow

1. **Inspect state:** current branch, `git status`, remotes, and working tree cleanliness. Do not discard/revert/overwrite unrelated user changes; investigate and report unexpected modifications.
2. **Identify base branch** (e.g. `main`/`master`) and assess local/remote CI health when possible (prefer `gh`, e.g. `gh run list --branch <base>`, `gh run view`; report concrete evidence). If CI/health cannot be determined, state the blocker and ask how to proceed.
3. **If baseline is green:** sync refs safely (`git fetch`, update local default branch if appropriate and safe; no destructive operations). Confirm required tooling/auth for intended work and report status.
4. **If baseline is red/failing:**
   - Treat baseline restoration as the priority item.
   - Handle it on a **separate baseline-fix branch/PR** (not mixed with unrelated feature work).
   - Implement/fix, validate (`validate-change`), push/open PR, and drive checks to green per repo flow.
   - **Pause for the baseline-fix PR to be merged** (human/maintainer action) before starting unrelated feature work. Do not build on top of a known-broken or unmerged baseline fix.
   - After merge, re-sync the healthy base and re-run preflight as needed.
5. **Dependencies/tooling:** never auto-upgrade dependencies unless explicitly requested/in-scope; use project-pinned mechanisms and explain necessity. Routine pinned installs needed to run checks are fine when required.

## Constraints

- Follow `global-gates` (safe Git, no destructive commands, evidence-based reporting). Report exact evidence and actionable blockers per root `AGENTS.md`.
