# R19 — In-browser COS simulator (Berry WASM)

- **ID:** r19-in-browser-cos-simulator
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R19, P3, not started), [`history/2026-11-05-in-browser-cos-simulator.md`](../../history/2026-11-05-in-browser-cos-simulator.md)
- **Approval:** pending

## Summary
Staged effort: pin Berry revision/config parity with firmware, build reproducible Berry WASM, run VM in dedicated Worker with watchdog/termination/recovery, define COS platform abstraction seam and canvas renderer, and integrate into IDE with explicit capability labels.

## Open questions
- [ ] Which stage(s) are in scope first and what are the acceptance criteria for each (toolchain spike, worker VM lifecycle, platform seam/renderer, input/timers/fs/commands, IDE integration)?
- [ ] Required fidelity/parity vs firmware and which hardware features are simulated vs stubbed/labeled?
- [ ] Build/reproducibility/tooling constraints (WASM toolchain, artifact storage/CI) and maintenance ownership?
- [ ] Performance/memory limits and UX expectations in the browser?

## Acceptance criteria
- [ ] Per-stage clarified specs with validation/evidence; staged delivery tracked (links to source spec).

## Validation plan
- Automated: frontend/worker/WASM build checks and tests as defined per stage.
- Manual: browser/IDE behavior validation per stage.

## Progress / evidence
- 2026-11-15: created (XL, staged; scoping/decisions required).
