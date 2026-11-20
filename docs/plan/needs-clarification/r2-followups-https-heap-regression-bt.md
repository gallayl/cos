# R2/R8 follow-ups — HTTPS PSA `-141` heap pressure, regression completion, BT coexistence

- **ID:** r2-followups-https-heap-regression-bt
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R2 follow-ups, R8 open follow-ups), [`reports/2026-11-08-r2-profiling-checklist.md`](../../reports/2026-11-08-r2-profiling-checklist.md), [`reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`](../../reports/2026-11-08-r8-http-stack-tls-measurement-guide.md), [`reports/2026-11-09-r8-followups-https-heap.md`](../../reports/2026-11-09-r8-followups-https-heap.md)
- **Approval:** pending (no implementation approval recorded; clarification decisions recorded 2026-11-20)

## Summary
Open follow-ups: resolve PSA/X.509 `PSA_ERROR_INSUFFICIENT_MEMORY` (`-141`) under heap pressure while keeping cert verification; complete remaining regression/clean-boot P7 checkpoints (repeated HTTP, HTTPS success when healthy, large/max response cases, etc.); measure/decide BT coexistence behavior with HTTPS/networking. Some timeout/abandon items validated in scope but optional captures remain.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Priority/order & mitigations:** **Fix `-141`, then regressions, then BT policy** (Recommended). Prioritize resolving PSA/X.509 `-141` with certificate verification enabled; complete required regression/clean-boot checks; then finalize BT coexistence behavior/policy from evidence. Accept temporary mitigations (e.g. request sequencing/policy, runtime BT release/disable when necessary) only if documented and evidence-backed. Certificate verification must remain enabled.
- **`-141` reproduction/done criteria & measurements:** **Clean-boot reproducible criteria with full context** (Recommended). Define done as: no `-141` in agreed clean-boot scenarios with cert verification enabled. Record BT state (on/off/connected), Wi‑Fi state, endpoint(s), request sequence, firmware image/commit/config, and heap metrics (`free`, `largest block`, `min watermark`, and internal/default heap metrics as relevant) before/during/after requests. Include at least one previously failing case and repeated runs showing stability. Acceptance = reproducible pass evidence or a documented, explicitly approved mitigation (verification still enabled).
- **Required regression cases/setup:** **Core R2/R8 follow-up set on CYD + known network** (Recommended). On supported CYD hardware with stable Wi‑Fi (record network/endpoint details): clean-boot P7 checkpoints still missing as applicable; repeated successful HTTP 200; HTTPS success on a valid endpoint when healthy with cert verification; large body near limit and max response; timeout/abandon sanity (no crash/stale data). Record heap and BT state. Additional cases optional/follow-up.
- **BT coexistence product decision:** **Goal concurrent support with evidence-backed constraints** (Recommended). Target concurrent BT + HTTPS/networking support. If evidence shows instability/`-141`/OOM, allow documented runtime constraints or mitigations (e.g. sequencing, temporary BT release/disable in specific flows) with clear UX/behavior notes. Decide final policy from measurements.

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current blockers (external):** requires investigation/fixes and **hardware/manual** network/HTTPS/TLS/heap/BT validation on supported CYD with ESP-IDF/tooling as applicable; evidence must be captured per decisions above. Final BT policy/mitigation acceptance depends on measured evidence.
- **Unblock/progress when:** (1) `-141` addressed or mitigated with reproducible clean-boot evidence (cert verification enabled) and recorded measurements/context; (2) core regression set executed with recorded results; (3) BT coexistence behavior measured and policy/constraints documented from evidence. Promote to ready/split into ready implementation slices only when specific work is fully specified, approved, and implementable now.

## Validation plan
- Automated: `make format-check`, `make test`; firmware build/lint and `make docs-check` as applicable for touched scope (per `AGENTS.md`).
- Hardware/manual (required): network/HTTPS/TLS, heap profiling, and BT scenarios on supported CYD with stable Wi‑Fi; record clean-boot context, BT/Wi‑Fi state, endpoints/request sequences, firmware/commit/config, heap metrics, and pass/fail evidence per decisions above.

## Progress / evidence
- 2026-11-15: created from open R2/R8 follow-ups (investigation/decision/hardware dependent).
- 2026-11-20 (clarify-feature, user): recorded decisions for priority/order+mitigations, `-141` clean-boot done criteria/measurements, required core regression set (CYD + known network), and BT coexistence policy direction (concurrent goal with evidence-backed constraints). Open questions cleared; item remains needs-clarification due to external investigation/hardware-validation blockers (not unanswered user questions).
