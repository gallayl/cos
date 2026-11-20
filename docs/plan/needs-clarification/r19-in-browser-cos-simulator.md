# R19 — In-browser COS simulator (Berry WASM)

- **ID:** r19-in-browser-cos-simulator
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R19, P3/M10, not started), [`history/2026-11-05-in-browser-cos-simulator.md`](../../history/2026-11-05-in-browser-cos-simulator.md)
- **Approval:** pending (clarification decisions/defaults recorded 2026-11-20; explicit approval required after a fully specified slice is promoted to `ready/`)

## Summary
Staged effort (per history spec): pin Berry revision/config parity with firmware, build reproducible Berry WASM, run VM in a dedicated Worker with watchdog/termination/recovery, define a COS platform abstraction seam and canvas renderer, and integrate into the IDE with explicit capability labels. Simulator is application-level (not ESP32/FreeRTOS/peripheral emulation) and must not claim hardware validation.

## Current context (reviewed)
- Consolidated plan lists R19 as P3/not started; no Berry WASM runtime, simulator Worker, platform abstraction backend, renderer, or simulator UI found in repo (history doc notes follow-ups/prerequisites).
- History spec proposes: exact Berry source revision/config parity, C API for VM lifecycle/GC/errors/memory, worker isolation, Emscripten config, platform services (display/clock/random/logging/input/timers/fs/commands/network/brightness), reuse COS UI `Renderer` where practical, staged implementation.

## Decisions (recorded 2026-11-20, clarify-feature, user-accepted suggested Stage 0 defaults)
- **Slice/scope (first):** **Stage 0 only** — reproducible Berry WASM build/toolchain spike with parity inputs pinned to firmware Berry revision/config (record exact rev/config source). Out of scope for this slice: COS platform seam, renderer, IDE integration, Worker runtime productization beyond minimal smoke harness needed to prove build.
- **Acceptance/deliverables/evidence (Stage 0 baseline):** documented reproducible build steps/commands; pinned Berry rev/config inputs identified; build artifacts produced in a defined location with documented artifact policy; minimal smoke validation that the built WASM can initialize/run a trivial Berry snippet (headless/node/browser harness acceptable for spike) with recorded results. Report in docs (spike report/plan update) with environment/versions/commands. Details may be refined when promoted to a ready slice but must be explicit before implementation.
- **Fidelity/capability labels:** Stage 0 validates build/parity-input feasibility only — **explicitly not hardware-validated**, no ESP32/FreeRTOS/peripheral emulation, and no COS UI/platform capability parity claimed.
- **Tooling/repro/ownership (baseline):** Emscripten version must be pinned (exact version recorded in slice spec/implementation); build scripts/config in a dedicated repo path (finalize in ready spec). Artifact/CI policy: generated WASM artifacts not committed by default unless explicitly decided in slice spec; CI integration optional for spike but reproducible local steps required. Ownership: contributor/maintainer who picks up the ready slice.
- **Perf/UX limits & blockers/prereqs (baseline):** browser perf/UX/watchdog N/A for headless Stage 0 except reasonable build/runtime smoke; record any build-time/resource constraints observed. Blockers/prereqs: access to pinned Berry sources/config, Emscripten toolchain, and frontend/build environment as applicable; sequencing — Stage 0 precedes Worker/platform/renderer/IDE stages.

## Open questions
- None (user-accepted Stage 0 defaults recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current state:** clarified for Stage 0 baseline but still needs a fully specified, explicitly approved **ready slice** (scope/non-goals, concrete AC/DoD, validation plan per `AGENTS.md`, expected areas/files, dependencies/prereqs, risks/constraints, approval) before implementation. External/practical blockers: pinned Berry sources/config availability, Emscripten toolchain setup, and maintainer ownership/capacity.

## Acceptance criteria
- [ ] (For future Stage 0 ready slice) Finalized Stage 0 spec meets Ready contract and records concrete deliverables/evidence per decisions above, with explicit non-hardware-validation labeling.

## Validation plan
- Automated (future slice, as applicable): frontend/build/script checks and any WASM/build/CI checks specified in the ready slice; run/report per `AGENTS.md` scope.
- Manual (future slice): smoke validation that built WASM initializes/runs a trivial Berry snippet (headless/node/browser harness acceptable) with recorded results.
- Hardware/on-device: not applicable except explicit "not validated on hardware" labeling (simulator does not replace L3 validation).

## Expected areas/files
- To be finalized in Stage 0 ready slice (likely dedicated build/tooling path for Berry WASM, docs/report, and possibly frontend/simulator scaffolding only if in scope).

## Dependencies / prerequisites
- Pinned Berry revision/config parity inputs, Emscripten toolchain (pinned in slice), build environment, and contributor/maintainer ownership for the ready slice.

## Risks / constraints
- P3/XL staged effort; parity and maintenance cost; avoid over-scoping first Ready item. WASM/toolchain reproducibility and artifact policy must be explicit.

## Progress / evidence
- 2026-11-15: created (XL, staged; scoping/decisions required).
- 2026-11-15: reviewed R19 needs-clarification, consolidated plan (R19), and `docs/history/2026-11-05-in-browser-cos-simulator.md`; recorded gaps and asked focused clarification questions in chat (fallback after `question` tool error). Item remained in `needs-clarification` pending user answers.
- 2026-11-20 (clarify-feature, user): selected Stage 0 (toolchain/parity spike) and accepted suggested defaults for acceptance/fidelity, tooling/repro/ownership, and perf/UX/blockers. Open questions cleared; item remains needs-clarification pending promotion to a fully specified, explicitly approved ready slice (not blocked by unanswered user questions).
