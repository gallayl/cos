# R23 — Frontend incremental dependency-aware compilation

- **ID:** r23-frontend-incremental-dependency-aware-compilation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (R23, partially in progress in IDE flow), `frontend/tools/compile-storage.ts`, `frontend/src/ide/import-specifiers.ts`, `frontend/src/ide/import-resolution.ts`, `frontend/src/editor/editor-app.tsx`
- **Approval:** pending

## Summary
Add dependency-aware, timestamp-based incremental compilation for relative TS imports across `compile-storage` and IDE flows (transitive deps, cycle handling, stale/missing `.be` detection) while preserving existing behavior.

## Open questions
- [ ] Current implementation status vs idea (IDE dialog/auto-compile in module mode done per notes?) — what remains in scope for a ready increment?
- [ ] Exact acceptance criteria for `compile-storage` integration (modes, `verifyStorage()` semantics) and for IDE deploy/run behavior?
- [ ] How to determine target/output mtimes and dependency freshness reliably across device/local contexts?
- [ ] Cycle/error reporting UX and required test coverage/documentation?

## Acceptance criteria
- [ ] Scoped increment clarified and approved with tests/docs; dependency-aware incremental behavior works for defined cases without regressing no-import files.

## Validation plan
- Automated: frontend `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` (and `npm run check-storage`/compile-storage tests as applicable) per `AGENTS.md`.

## Progress / evidence
- 2026-11-15: created from ideas (partially implemented; scope/remaining work needs clarification).
