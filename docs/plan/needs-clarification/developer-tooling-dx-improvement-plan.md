# Developer tooling & DX improvement plan — triage to ready items

- **ID:** developer-tooling-dx-improvement-plan
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-11-developer-tooling-dx-improvement-plan.md`](../2026-11-11-developer-tooling-dx-improvement-plan.md) (primary source), [`reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md), [`ideas/2026-11-11-unified-agent-structure.md`](../../ideas/2026-11-11-unified-agent-structure.md), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md), [`docs/README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), workflows in `.github/workflows/`
- **Approval:** pending (none recorded)

## Summary
The 2026-11-11 DX improvement plan defines 7 workstreams (DX1–DX7) with goals, actions, risks, and a progress tracker. Some quick wins/measurements are recorded, but the plan is not triaged into queue items that satisfy the docs **Ready contract** (no open questions, explicit approval after specification, concrete per-item scope/non-goals/acceptance/validation/dependencies/risks/areas). This item tracks clarification/triage: split actionable pending work into separate ready items (or keep/record explicit deferrals/blockers) instead of moving this umbrella doc to `plan/ready/`.

## Context reviewed (2026-11-15, clarification pass)
- `docs/README.md` — Ready vs needs-clarification queues and Ready contract.
- `docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md` — goals/non-goals, pain points, DX1–DX7 actions/acceptance, suggested order, open questions/decisions, progress tracker.
- `docs/reports/2026-11-11-dx1-baseline-measurements.md` — initial local baselines (B1–B8 scripts/frontend/host tests), firmware build + local ccache comparison (FB1–FB4), blockers/gaps (Berry compiler, integration build/run baselines, lint, cross-platform, peak RAM/disk/log-volume, CI comparison).
- `docs/ideas/2026-11-11-unified-agent-structure.md` — shared foundation/skills/commands strategy (marked implemented direction in repo context; see relation notes below).
- `docs/plan/consolidated-improvement-plan.md` — no dedicated R-item for DX; 2026-11-15 entry notes DX triage item created (restructuring/triage only).
- Current repo signals: `.agents/skills/` and harness adapters/commands exist; CI workflows present (`build-and-lint.yml`, `docs-check.yml`, `format-check.yml`, `frontend.yml`, `integration-test.yml`, `test.yml`); `docs/plan/ready/` currently contains only `r24-berry-script-http-server-api.md`.

## Triage assessment (current state vs Ready contract)

| Workstream | Current evidence/state (from sources) | Ready-contract gaps / notes |
|---|---|---|
| **DX1 — Baseline metrics and observability** (P0) | In progress. Local script/host/frontend baselines + firmware build/ccache comparison recorded. Missing: lint/integration build-run baselines, CI runner comparison, cross-platform, peak RAM/disk/log volume, lightweight timing/summary collection mechanism, documented CI assumptions alongside results. | Needs selected remaining scope (which scenarios/platforms/environments are “enough” now), concrete acceptance criteria, validation commands/artifacts, dependencies (ESP-IDF/QEMU/CI access), explicit approval. May be split (e.g. “finish baseline report in canonical env/CI” vs “add timing/summary tooling”). |
| **DX2 — Reproducible environment & dependency simplification** (P0) | Not started per tracker. Large surface (devcontainer canonical path, bootstrap scripts PS/Bash, Python workflow/uv vs pip-tools, frontend Corepack/Yarn alignment, generated/tracked artifact policy for `sdkconfig`/`managed_components`/`frontend/dist`/build outputs, env template/detection). | Open decisions in source (Python manager vs `IDF_PYTHON`; artifact source-control/freshness policy). Must be split into scoped ready items with explicit scope/non-goals, cross-platform expectations, CI parity, and validation per `AGENTS.md`. High approval/decision load. |
| **DX3 — Build and lint performance** (P1) | In progress. Local `IDF_CCACHE_ENABLE` + ccache speedup measured; Makefile `SOURCES` deterministic discovery done (2026-11-12). Remaining: repo/CI compiler cache integration, parallel build ergonomics, lint pipeline reuse/scoped changed-files mode, SARIF/JUnit summaries (optional). | Each candidate (e.g. CI ccache/cache dirs, changed-files lint mode, lint DB reuse) needs its own scope/acceptance/validation and explicit decision where source flags choices (“changed-files lint advisory vs required gate”). |
| **DX4 — Test and integration-test throughput** (P1) | In progress. `INTEGRATION_TEST_JOBS` exposed in Makefile/docs. Remaining: safe default concurrency (dev/CI), safe parallel test execution, first-class selection UX, host-test loop ergonomics, strategic consolidation investigation. | Source open question: safe parallelism defaults and feasibility of consolidating integration projects. Needs per-item scoping and risk/validation evidence (no loss of required CI coverage). |
| **DX5 — Output ergonomics and failure signal** (P1) | Not started. | Needs concrete UX/spec for verbosity modes/wrappers, CI grouping/annotations, pytest/tidy defaults, command echoing/exit-code taxonomy — and decision on defaults vs opt-in to avoid hiding embedded/QEMU diagnostics. |
| **DX6 — Developer workflow UX and guardrails** (P2) | Not started. | Needs decision on task runner (`just`/Taskfile/etc.), hook policy, doc locations, and scope boundaries (Make targets remain canonical for CI). |
| **DX7 — CI efficiency and parity** (P2) | In progress. Integration-test workflow caches `integration_test/test_*/build`; ccache/pip notes in source; local evidence supports compiler caching in build-and-lint/integration. Remaining: broader workflow caching, parallel builds/runs where safe, align quiet/summary modes with artifacts, image/toolchain freshness review. | Needs per-workflow scoped items with cache-key correctness/invalidation plan and CI-equivalent validation evidence. |

## Relation to current agent/skills/commands structure and CI/tooling constraints (to refine with user)
- **Agent structure (implemented direction):** repo now has shared `AGENTS.md` (+ nested where applicable), `.agents/skills/` layered procedures, and harness commands/adapters (see unified-agent-structure idea). DX items should **avoid duplicating** workflow/procedure docs: prefer updating canonical `AGENTS.md`, `docs/README.md`, `README.md`, Make targets/scripts, and/or focused skills only when a repeatable DX procedure is warranted. Any DX work that proposes new “how to work” procedures must stay consistent with existing skills/commands and the Ready/needs-clarification queue model.
- **CI/tooling constraints (must respect per `AGENTS.md` and workflows):** CI-aligned checks include `make format-check`, `make test`, frontend `yarn format:check/lint/test/build`, `make build`/`make lint` in ESP-IDF v6.1 environments, `make docs-check`, and integration build/run flows (`scripts/integration_test.py`, QEMU, pinned `integration_test/requirements.txt`). ESP-IDF/QEMU/clang/Node24+Yarn requirements, multi-platform (Windows/Linux/macOS) considerations, USB/native flashing constraints, and “do not claim unrun checks” reporting rules constrain DX changes. Cache/parallelism/output changes must preserve required coverage and debuggability.

## Open questions (need user decisions to triage/satisfy Ready contract)

1. **Prioritization / selection:** Which DX outcomes matter most **now**? Pick one path:
   - (A) Finish **DX1** baselines/evidence first (including integration/lint/CI/cross-platform gaps you care about), then implement performance/CI items against measured deltas.
   - (B) Prioritize **DX3/DX4/DX7** throughput/CI quick wins (ccache in CI, safe parallel defaults, selection UX) even with partial baselines.
   - (C) Prioritize **DX2** environment/bootstrap simplification first.
   - (D) Prioritize **DX5/DX6** ergonomics/workflow UX.
   - (E) Defer the whole DX umbrella for now (record rationale) and only track ad-hoc fixes elsewhere.
   - (F) Other (specify).

2. **Granularity for ready queue:** Should this be triaged into **multiple separate ready items** (recommended: one per independently approvable/deliverable slice, e.g. “Add ccache to build-and-lint CI workflow”, “Document/measure integration-test baseline in ESP-IDF+QEMU env”), or do you want a smaller number of bundled items? Which specific slices should be created first?

3. **Scope decisions inherited from DX plan (per selected slice):** for each item you want promoted to ready, please provide/confirm: **scope & non-goals**, **acceptance criteria/DoD**, **validation plan** (exact commands/environments/platforms; what can/cannot be run automatically), **expected areas/files**, **dependencies/prerequisites**, **risks/constraints**, and any platform exclusions/“not yet validated on X”.

4. **Key open decisions from source plan:** which choices apply to prioritized items?
   - Python manager for repo tooling (`uv` vs `pip-tools`+venv vs status quo) and relation to `IDF_PYTHON`.
   - Policy for `sdkconfig`, `managed_components`, `frontend/dist`, build outputs/caches: source-control, generated-only, or freshness checks (and where).
   - Safe default parallelism for integration builds/tests on CI runners vs developer machines.
   - “Changed-files lint”: advisory local check vs required gate.
   - Task runner for DX6 (`just`, Taskfile, Make-only wrappers, or none).

5. **Approval / Ready transition:** `plan/ready/` items must be **approved after specification**. Once we draft complete specs for selected slices with **no open questions**, will you explicitly approve (a) each slice individually and (b) moving each approved slice to `docs/plan/ready/<item-id>.md` as pre-approved for implementation/PR flow (merge remains human)? If not, I will keep items in `needs-clarification` with recorded decisions/blockers.

## Acceptance criteria (for this triage item)
- [ ] Pending DX work triaged: each actionable item either defined as a separate ready item in `docs/plan/ready/` with full Ready-contract fields and explicit recorded approval, or explicitly deferred/kept in `docs/plan/needs-clarification/` with open questions or blocker/deferral rationale and what would unblock it.
- [ ] `docs/plan/consolidated-improvement-plan.md` cross-links/status notes kept consistent if any DX queue items are added/moved.

## Validation plan
- **This clarification pass:** docs-only review/update; no runtime/build validation required.
- **Per future clarified implementation item:** follow `AGENTS.md` by scope (e.g. `make format-check`, `make test`; frontend `yarn format:check/lint/test/build`; `make build`/`make lint`/`make docs-check`/integration targets as applicable and feasible). Cross-platform claims must be verified or explicitly marked unvalidated. Exact commands/results to be recorded in the promoted ready item(s).

## Progress / evidence
- 2026-11-15: created (plan requires triage/clarification into ready vs needs-clarification items).
- 2026-11-15 (clarify-feature pass): reviewed target/source/report/idea/consolidated plan, current queues, skills/adapters, and CI workflows; recorded workstream-by-workstream triage assessment, relation to implemented agent structure and CI/tooling constraints, and focused open questions/decisions needed for Ready-contract compliance. No items promoted to `ready/` (no per-slice specs + no explicit approval recorded). Status remains **needs-clarification**.
