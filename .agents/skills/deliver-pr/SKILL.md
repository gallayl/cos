---
name: deliver-pr
description: Granular procedure to commit focused changes, push a branch, and open or update a pull request with minimal, doc-linked description. Use when composing implementation workflows; never merges the PR.
---

# Deliver PR (granular)

Package validated, scoped work into commits and a PR. **Granular skill** composed by primary/repo-specific workflows. **Never merge the PR.**

## Preconditions

- Scope implemented and relevant local validation in a good state (or gaps clearly documented) per `validate-change` / root `AGENTS.md`.
- Correct working branch prepared from agreed healthy base (`prepare-working-branch`).
- Required progress/evidence tracked in docs as applicable (`track-doc-progress`).

## Workflow

1. **Commit (if appropriate):** stage only focused, in-scope files (no unrelated changes). Use clear **Conventional Commits** messages (e.g. `feat:`, `fix:`, `docs:`, `test:`, `chore:`; include scope when useful). Multiple logical commits are fine.
2. **Push** the working branch to the remote when credentials/tooling are available.
3. **Open or update PR** against the agreed base branch. Prefer **GitHub CLI (`gh pr create` / `gh pr edit`)** when available. Prefer a **draft PR** while work/CI is in progress if supported; mark ready when up to date and checks are green.
4. **PR description (minimal):** short summary/scope and **links to relevant documentation** (plan/idea/history/report) containing detailed progress and validation evidence. Call out required hardware/manual validation, known limitations, and blockers when applicable. Keep description up to date with PR state. If hardware/manual validation is required, add a clear PR comment or review thread stating what must be validated and treat it as a merge-blocking item until resolved with evidence (or explicitly waived by a human with rationale). Do not claim unrun checks or hardware validation.
5. **If blocked** (auth/permissions/missing `gh`/network): report evidence and provide exact fallback commands/instructions.

## Constraints

- No force-push/history rewrites, no branch deletion, no merging. Follow root `AGENTS.md` safety/reporting rules.
- After opening, CI/review iteration is handled by `babysit-pr` (or equivalent) as a composed step when in scope.
