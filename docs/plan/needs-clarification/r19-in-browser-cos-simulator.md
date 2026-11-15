# R19 — In-browser COS simulator (Berry WASM)

- **ID:** r19-in-browser-cos-simulator
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R19, P3/M10, not started), [`history/2026-11-05-in-browser-cos-simulator.md`](../../history/2026-11-05-in-browser-cos-simulator.md)
- **Approval:** pending (explicit approval required after specification to move to `ready/`)

## Summary
Staged effort (per history spec): pin Berry revision/config parity with firmware, build reproducible Berry WASM, run VM in a dedicated Worker with watchdog/termination/recovery, define a COS platform abstraction seam and canvas renderer, and integrate into the IDE with explicit capability labels. Simulator is application-level (not ESP32/FreeRTOS/peripheral emulation) and must not claim hardware validation.

## Current context (reviewed)
- Consolidated plan lists R19 as P3/not started; no Berry WASM runtime, simulator Worker, platform abstraction backend, renderer, or simulator UI found in repo (history doc notes follow-ups/prerequisites).
- History spec proposes: exact Berry source revision/config parity, C API for VM lifecycle/GC/errors/memory, worker isolation, Emscripten config, platform services (display/clock/random/logging/input/timers/fs/commands/network/brightness), reuse COS UI `Renderer` where practical, staged implementation.

## Spec gaps / Ready-contract blockers
- **Scope and non-goals (slice):** which stage(s)/deliverables are in scope for this item now vs later R19 follow-ups.
- **Acceptance criteria / DoD:** concrete, testable criteria per in-scope stage/deliverable.
- **Validation plan (automated/manual):** specific frontend/worker/WASM/build/test commands and browser/manual checks feasible per `AGENTS.md` for the chosen slice.
- **Expected areas/files:** likely touch points for the chosen slice (build/tooling, frontend/simulator worker/runtime, docs/assets) not yet defined.
- **Dependencies/prerequisites:** Berry source pinning/build inputs, Emscripten/toolchain versioning, CI/artifact decisions, platform seam decisions.
- **Risks/constraints:** parity drift, WASM/toolchain reproducibility, worker/main-thread boundaries, memory/perf limits, maintenance ownership.
- **Approval:** no explicit approval recorded.

## Open questions
- [ ] **Slice/scope:** Which stage(s) should this clarification make Ready first (e.g. Stage 0 toolchain spike, 0–1 runtime foundation, MVP through renderer/IDE integration, or backlog-only refinement)?
- [ ] **Acceptance criteria:** For the chosen slice, what exact deliverables/evidence define done (reproducible build artifacts? headless worker API? diagnostics? CI job? browser demo page/IDE integration)?
- [ ] **Fidelity/capabilities:** Required parity vs firmware for this slice and which features are simulated vs stubbed/explicitly "not hardware-validated"?
- [ ] **Tooling/reproducibility:** Emscripten version/pinning, where WASM build lives, artifact storage/commit policy, CI integration, and maintenance ownership.
- [ ] **Performance/UX limits:** Browser memory/CPU expectations, watchdog/timeout defaults, and UX scope for this slice.
- [ ] **Dependencies/risks:** Any hard blockers or sequencing constraints that must be captured as prerequisites.

## Decisions log
- 2026-11-15: Initial clarification pass started. `question` tool invocation failed due to malformed arguments; questions asked in plain chat (awaiting user response). No requirements/approval invented.

## Acceptance criteria
- [ ] (Pending) Per-stage clarified specs with validation/evidence for the approved in-scope slice; later stages tracked explicitly.

## Validation plan
- Automated: (pending scope) expected frontend checks per `AGENTS.md` for frontend changes; WASM/worker/build checks/tests to be specified.
- Manual: (pending scope) browser/IDE behavior validation per in-scope stage.
- Hardware/on-device: not applicable except explicit "not validated on hardware" labeling (simulator does not replace L3 validation).

## Expected areas/files
- (Pending scope decision)

## Dependencies / prerequisites
- (Pending scope/tooling decisions) Berry revision/config parity inputs, WASM toolchain availability, frontend build/test environment.

## Risks / constraints
- P3/XL staged effort; parity and maintenance cost; avoid over-scoping first Ready item.

## Progress / evidence
- 2026-11-15: created (XL, staged; scoping/decisions required).
- 2026-11-15: reviewed R19 needs-clarification, consolidated plan (R19), and `docs/history/2026-11-05-in-browser-cos-simulator.md`; recorded gaps and asked focused clarification questions in chat (fallback after `question` tool error). Item remains in `needs-clarification` pending user answers.
