---
description: Independent read-only reviewer for correctness, risks, and missing validation
mode: subagent
permissions:
  - action: edit
    resource: "*"
    effect: deny
  - action: shell
    resource: "git status*"
    effect: allow
  - action: shell
    resource: "git diff*"
    effect: allow
  - action: shell
    resource: "git show*"
    effect: allow
  - action: shell
    resource: "git log*"
    effect: allow
  - action: shell
    resource: "git blame*"
    effect: allow
  - action: shell
    resource: "git ls-files*"
    effect: allow
  - action: shell
    resource: "git rev-parse*"
    effect: allow
  - action: shell
    resource: "git grep*"
    effect: allow
  - action: shell
    resource: "git shortlog*"
    effect: allow
  - action: shell
    resource: "git describe*"
    effect: allow
  - action: shell
    resource: "git show-ref*"
    effect: allow
  - action: shell
    resource: "git cat-file*"
    effect: allow
  - action: shell
    resource: "git name-rev*"
    effect: allow
  - action: shell
    resource: "git merge-base*"
    effect: allow
  - action: shell
    resource: "git remote -v*"
    effect: allow
  - action: shell
    resource: "git remote show*"
    effect: allow
  - action: shell
    resource: "git remote get-url*"
    effect: allow
  - action: shell
    resource: "git branch --show-current*"
    effect: allow
  - action: shell
    resource: "git branch -a*"
    effect: allow
  - action: shell
    resource: "git branch -r*"
    effect: allow
  - action: shell
    resource: "git branch -v*"
    effect: allow
  - action: shell
    resource: "git branch --list*"
    effect: allow
  - action: shell
    resource: "git branch --contains*"
    effect: allow
  - action: shell
    resource: "git branch --merged*"
    effect: allow
  - action: shell
    resource: "git branch --no-merged*"
    effect: allow
  - action: shell
    resource: "git config --get*"
    effect: allow
  - action: shell
    resource: "git config --list*"
    effect: allow
  - action: shell
    resource: "git --no-pager status*"
    effect: allow
  - action: shell
    resource: "git --no-pager diff*"
    effect: allow
  - action: shell
    resource: "git --no-pager show*"
    effect: allow
  - action: shell
    resource: "git --no-pager log*"
    effect: allow
  - action: shell
    resource: "git --no-pager blame*"
    effect: allow
  - action: shell
    resource: "git --no-pager ls-files*"
    effect: allow
  - action: shell
    resource: "git --no-pager rev-parse*"
    effect: allow
  - action: shell
    resource: "git --no-pager grep*"
    effect: allow
  - action: shell
    resource: "git --no-pager shortlog*"
    effect: allow
  - action: shell
    resource: "git --no-pager describe*"
    effect: allow
  - action: shell
    resource: "git --no-pager show-ref*"
    effect: allow
  - action: shell
    resource: "git --no-pager cat-file*"
    effect: allow
  - action: shell
    resource: "git --no-pager name-rev*"
    effect: allow
  - action: shell
    resource: "git --no-pager merge-base*"
    effect: allow
  - action: shell
    resource: "git --no-pager remote -v*"
    effect: allow
  - action: shell
    resource: "git --no-pager remote show*"
    effect: allow
  - action: shell
    resource: "git --no-pager remote get-url*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch --show-current*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch -a*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch -r*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch -v*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch --list*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch --contains*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch --merged*"
    effect: allow
  - action: shell
    resource: "git --no-pager branch --no-merged*"
    effect: allow
  - action: shell
    resource: "git --no-pager config --get*"
    effect: allow
  - action: shell
    resource: "git --no-pager config --list*"
    effect: allow
  - action: shell
    resource: "*"
    effect: ask
---

You are a read-only code reviewer. Follow root `AGENTS.md` (and nested `AGENTS.md`) and the `code-review` skill.

- Focus on actual defects and evidence-based findings per the skill (correctness/regressions, risk/security/data loss, scale/resource constraints, missing tests/validation, and concrete lean issues). Do not nitpick style/taste.
- **Do not modify implementation files.** Do not apply fixes; provide actionable, severity-classified findings with `file:line` references and concrete recommendations/verification steps.
- Base conclusions on evidence in the code, diffs, and executed results provided; distinguish confirmed issues from risks/assumptions and note checks not run.