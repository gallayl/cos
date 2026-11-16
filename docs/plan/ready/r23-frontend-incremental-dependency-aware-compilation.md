# R23 — Frontend incremental dependency-aware compilation (`compile-storage`)

- **ID:** r23-frontend-incremental-dependency-aware-compilation
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Updated:** 2026-11-15 (clarified and approved)
- **Ready:** 2026-11-15
- **Source/links:** [`../needs-clarification/r23-frontend-incremental-dependency-aware-compilation.md`](../needs-clarification/r23-frontend-incremental-dependency-aware-compilation.md) (clarification history, moved), [`../../ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (R22 idea entry, partially in progress in IDE flow), [`../../README.md`](../../README.md), root [`../../../AGENTS.md`](../../../AGENTS.md), `frontend/tools/compile-storage.ts`, `frontend/src/ide/import-specifiers.ts`, `frontend/src/ide/import-resolution.ts`, `frontend/src/editor/editor-app.tsx`, `frontend/tools/compile-storage.test.ts`
- **Approval:** User explicitly approved in chat on 2026-11-15 ("Approve and move to ready"). **Pre-approved for implementation/PR flow** (merge remains human/maintainer action). No implementation performed in this clarification pass.

## Open questions
- None.

## Summary
Add dependency-aware, timestamp-based incremental compilation for relative TypeScript imports in `frontend/tools/compile-storage.ts` (local storage build/check tool), while preserving correctness guarantees and existing behavior for files without relative imports. IDE already has partial dependency discovery/mtime checks and optional auto-compile/write of outdated/missing deps in module mode (per idea/editor flow); this increment scopes `compile-storage` only (shared utilities may be refactored/reused if needed, but no IDE UX/behavior changes required).

## Current state / context (reviewed 2026-11-15)
- `storage/ts/**/*.ts` sources compile to `storage/flash_data/**/*.be` (mirrored relative paths). `compile-storage` collects all `.ts` and `.be`, typechecks via `storage/tsconfig.json`+SDK, rejects JS globals, compiles each TS independently with `astToBerry(getAst(...))`, and `verifyStorage()` compares **semantic token equivalence** for all TS-backed scripts against committed `.be` (`tests/` hand-written Berry scripts without TS sources are excluded from TS-backed checks per current logic). `--write` currently compiles and writes **all** TS files.
- `frontend/src/ide/import-specifiers.ts` extracts relative import specifiers (`./`, `../`) from TS source. `frontend/src/ide/import-resolution.ts` resolves relative specifiers to device TS paths (adds `.ts` when no extension). Editor compile flow (`frontend/src/editor/editor-app.tsx`) already performs transitive relative-dep discovery, checks device `.ts` vs `.be` mtimes, and can auto-compile/write outdated/missing deps in module mode (partial IDE implementation).
- Berry resolves relative imports at runtime from `.be` files; a compiled `.be` file's content depends on its own `.ts` source.

## Decisions / resolved questions (2026-11-15, user answers)
- [x] **Increment scope:** `compile-storage` dep-aware incremental compile only (no IDE completion/polish required in this increment).
- [x] **`compile-storage` semantics:** `--write` becomes dependency-aware and incremental: build transitive relative-import dependency graph and compile/write stale/missing outputs using **per-file freshness** (see acceptance criteria). `verifyStorage()` remains a **full correctness check** that compiles all TS in-memory and verifies semantic equivalence vs committed `.be` for all required scripts (must not skip required verification based on freshness/mtimes). Behavior for files with no relative imports preserved.
- [x] **Freshness/rebuild rule:** per-file freshness only. For each relevant `storage/ts/**/*.ts`, compile/write when its own output `.be` is missing or has mtime older than that file's `.ts` mtime. Do **not** force recompiling an unchanged importer solely because a dependency changed/recompiled. Transitive graph is still discovered/validated (including deps) for cycle detection and to ensure referenced TS deps are known/handled.
- [x] **Cycles / errors / tests:** cycles in relative-import graph must produce deterministic, actionable errors (identify cycle path/specifiers) with no infinite recursion in flows in scope. Add/update unit tests covering dependency discovery, transitive deps, missing/stale `.be`, unchanged no-import behavior, and cycle/error cases. Update docs/comments only as needed. `npm run check-storage` / compile-storage tests remain passing.
- [x] **Validation:** automated only, no hardware/manual validation required. Frontend checks from `frontend/` + storage/compile-storage checks as specified below. No firmware build/lint/integration/docs-check required unless implementation scope changes.

## Scope
- `frontend/tools/compile-storage.ts`: dependency-aware incremental behavior for `--write` over `storage/ts` (local filesystem) with per-file mtime freshness, transitive relative-import graph discovery, and cycle detection/errors.
- Ensure `verifyStorage()` continues to perform full semantic verification for all TS-backed storage scripts (existing correctness contract), compatible with dependency-aware compilation outputs.
- Reuse/extract existing import specifier/resolution helpers where practical for consistency (e.g. `collectRelativeImportSpecifiers`, relative path resolution rules appropriate for local storage paths), without changing IDE behavior.
- Tests for the new/changed compile-storage behavior and graph/freshness/cycle logic.
- Minimal docs/comments updates if needed to describe new `--write` behavior/error cases.

## Non-goals
- IDE compile/deploy/run UX changes, device-mtime handling, or completion of partial IDE dependency features (tracked separately by R23 idea progress).
- Changing Berry runtime import semantics or adding non-relative module dependency tracking (`@cos/*`, `@berry/*`, absolute/non-relative imports, type-only imports already stripped/ignored as applicable).
- Freshness-only or partial `verifyStorage()` that skips required semantic verification.
- Hardware/firmware/QEMU validation.
- Broad refactors unrelated to compile-storage incremental dependency handling.

## Acceptance criteria / definition of done
- [x] `compile-storage` discovers relative TS imports (`./`, `../`) and resolves them transitively to `storage/ts` source paths for dependency-graph/cycle analysis (ignoring non-relative imports and resolving per existing local relative behavior; type-only handling unchanged via source scanning/compiler behavior).
- [x] Relative-import cycles produce deterministic, actionable errors identifying the cycle (path/specifiers) and fail without infinite recursion.
- [x] `compile-storage --write` is incremental and dependency-aware with **per-file freshness**: for each `storage/ts/**/*.ts` file in scope (including transitive relative deps), compile and write `storage/flash_data/<name>.be` when the output is missing or older than that `.ts` file's mtime; skip writing when the output is up-to-date. Unchanged importers are not forcibly recompiled solely due to dependency recompilation. Files with no relative imports preserve existing compile/write behavior (subject to the same per-file freshness rule).
- [x] `verifyStorage()` remains a full semantic correctness check for all required TS-backed storage scripts (compiles TS and compares semantic tokens vs committed `.be` as today, or equivalent) and is not weakened to freshness-only checks. Existing `tests/` hand-written Berry-script handling remains correct.
- [x] Error reporting is clear for missing/unresolvable relative deps, cycles, and compile diagnostics (consistent/actionable).
- [x] Unit tests added/updated covering: dependency discovery, transitive deps, missing `.be`, stale `.be` (older mtime), up-to-date skip, unchanged no-import behavior, and cycle/error cases. Existing compile-storage/storage checks continue to pass.
- [x] No regressions to storage script compilation/verification semantics outside this scoped behavior.

## Validation plan
- **Automated (required, run from `frontend/` unless noted):**
  - `yarn format:check`
  - `yarn lint`
  - `yarn test` (includes compile-storage/unit tests)
  - `yarn build`
  - `npm run check-storage` (must pass; `verifyStorage()` performs full semantic equivalence checks for all required scripts)
- **Hardware/manual:** none required.
- **Out of scope unless implementation changes:** `make build`, `make lint`, `make integration-test*`, `make docs-check`.

## Expected areas/files
- `frontend/tools/compile-storage.ts` (incremental `--write`, dependency graph/cycle detection, freshness checks, integration with existing verify/compile paths)
- `frontend/tools/compile-storage.test.ts` and/or new focused tests/fixtures under `frontend/tools/` or `frontend/src/` as appropriate
- Possibly shared helpers in `frontend/src/ide/import-specifiers.ts`, `frontend/src/ide/import-resolution.ts`, or a small local/storage-oriented helper (only if needed for reuse; keep IDE behavior unchanged)
- Docs/comments in touched files as needed

## Dependencies / prerequisites
- Node.js 24.x and Yarn available for frontend checks; `yarn install --frozen-lockfile` in `frontend/` if deps missing/stale.
- Local filesystem mtimes for `storage/ts` and `storage/flash_data` used for freshness comparison (no device access).

## Risks / constraints
- Filesystem mtime granularity/resolution can affect freshness decisions in edge cases; implementation should be deterministic/reasonable (e.g. treat equal/older output per chosen comparison consistently) and covered by tests with controlled mtimes where feasible.
- Path normalization/resolution must correctly map relative specifiers (including `..`) to `storage/ts` files and corresponding `.be` outputs without escaping roots or mis-resolving extensions.
- `--write` skipping up-to-date files changes write behavior but must not change resulting `.be` correctness; `verifyStorage()` remains the authoritative full semantic check.
- Avoid introducing IDE regressions if shared modules are refactored.

## Progress / evidence
- 2026-11-15: created from ideas (partially implemented; scope/remaining work needs clarification).
- 2026-11-15 (clarify-feature pass, round 1): selected R23 as next clarification target (r20-pipes-runtime-v1 already in progress). Reviewed idea/source files and current `compile-storage`/IDE import handling. User decisions: scope compile-storage dep-aware incremental compile only (defer IDE completion); `--write` incremental with per-file freshness + transitive dep graph/cycle detection, `verifyStorage()` remains full semantic verification for all; cycles must be deterministic/actionable with no infinite recursion; add/update unit tests for discovery/transitive/missing/stale/up-to-date/no-import/cycle cases; validation = `frontend` `yarn format:check/lint/test/build` + `npm run check-storage` (automated only, no hardware).
- 2026-11-15 (clarify-feature pass, round 2): user confirmed **per-file freshness only** (do not recompile unchanged importers solely due to dependency recompilation) and **frontend + storage checks only** validation. Drafted complete Ready-contract spec fields with open questions = none.
- 2026-11-15 (approval): user explicitly approved spec and move to `docs/plan/ready/` as **ready/pre-approved for implementation/PR flow** (merge remains human). Item moved to ready. No implementation performed.
- 2026-11-16 (implementation, `feat/r23-compile-storage-incremental`): implemented dependency-aware incremental `compile-storage --write` with per-file mtime freshness, transitive relative-import graph discovery/validation, missing-dep and cycle detection (actionable errors), and added/updated unit tests in `frontend/tools/compile-storage.test.ts`. `verifyStorage()` remains full semantic verification. Local validation from `frontend/`: `yarn format:check` PASS, `yarn lint` PASS, `yarn test` PASS (19 files, 136 tests), `yarn build` PASS (existing chunk-size warnings), `npm run check-storage` PASS (1 file, 7 tests). No hardware/manual validation required. PR/CI pending at time of implementation; move to `docs/plan/done/` after PR opened/updated and required checks/PR CI green per scope.
