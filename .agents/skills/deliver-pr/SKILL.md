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
3. **Open or update PR** against the agreed base branch. Prefer **GitHub CLI (`gh pr create` / `gh pr edit` / `gh pr ready`)** when available. Prefer a **draft PR** while work/CI is in progress if supported; mark ready when up to date and checks are green.
   - **Create/update PR body safely:** do **not** pass multi-line markdown inline in shell commands (heredocs/inline `--body` often fail or introduce escaping issues in this environment). Write the full description to a temporary markdown file and use `gh pr create --body-file <path>` / `gh pr edit --body-file <path>` (or equivalent). Use plain Markdown with normal backticks/links; avoid manual escaping and verify the rendered/sent body has no unintended escape sequences (e.g. stray backslashes before paths/commands).
4. **PR description (minimal, factual):** short summary/scope and **links to relevant documentation** (plan/idea/history/report) containing detailed progress and validation evidence. Keep it up to date with PR state.
   - **Validation:** report only checks **actually executed locally** with exact commands and pass/fail outcomes. Do **not** claim unrun checks or hardware validation. Do **not** list CI-covered checks (e.g. `make build`, `make lint`, or other pipeline-run checks) as local validation unless you ran them and reporting them is necessary; rely on CI for them and note CI/check status separately when known.
   - Call out required hardware/manual validation, known limitations, blockers, and remaining human actions when applicable. If hardware/manual validation is required, add a clear PR comment or review thread stating what must be validated and treat it as merge-blocking until resolved with evidence (or explicitly waived by a human with rationale).
5. **If blocked** (auth/permissions/missing `gh`/network): report evidence and provide exact fallback commands/instructions.

## Constraints

- Follow `global-gates` and root `AGENTS.md` (safe Git, no branch deletion/merge, evidence-based reporting). No force-push/history rewrites.
- After opening, CI/review iteration is handled by `babysit-pr` (or equivalent) as a composed step when in scope. The composing workflow must invoke/start `babysit-pr` immediately after PR open/update in the same run and continue until merge-ready or blocked; do not treat PR creation as completion.
