---
name: prepare-working-branch
description: Granular procedure to create or update a dedicated working branch from an agreed healthy base, safely syncing with the base branch when needed. Use when composing implementation/PR workflows.
---

# Prepare working branch (granular)

Prepare a safe, dedicated branch for scoped work. **Granular skill** composed by primary/repo-specific workflows.

## Workflow

1. **Preconditions:** baseline is understood/healthy (see `ensure-green-baseline`) and the agreed base branch/commit is identified. Ensure working tree state is understood; avoid carrying unrelated local changes.
2. **Create/switch branch** from the up-to-date, healthy base (feature branch for feature work, or a **separate baseline-fix branch** for baseline restoration). Use a clear branch name aligned with repo conventions (or a sensible default).
3. **Keep up to date safely:** if the base moves or branch becomes stale, update by **merging the base branch into the working branch** (preferred). Resolve conflicts carefully and re-run relevant validation.
4. **Safety:** no rebase/force-push unless explicitly required and approved; no destructive operations; do not discard unrelated user changes.

## Output

- Report base, branch name, sync actions taken, and any blockers/conflicts requiring decisions.
