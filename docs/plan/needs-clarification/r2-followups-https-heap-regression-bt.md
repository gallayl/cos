# R2/R8 follow-ups — HTTPS PSA `-141` heap pressure, regression completion, BT coexistence

- **ID:** r2-followups-https-heap-regression-bt
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R2 follow-ups, R8 open follow-ups), [`reports/2026-11-08-r2-profiling-checklist.md`](../../reports/2026-11-08-r2-profiling-checklist.md), [`reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`](../../reports/2026-11-08-r8-http-stack-tls-measurement-guide.md), [`reports/2026-11-09-r8-followups-https-heap.md`](../../reports/2026-11-09-r8-followups-https-heap.md)
- **Approval:** pending

## Summary
Open follow-ups: resolve PSA/X.509 `PSA_ERROR_INSUFFICIENT_MEMORY` (`-141`) under heap pressure while keeping cert verification; complete remaining regression/clean-boot P7 checkpoints (repeated HTTP, HTTPS success when healthy, large/max response cases, etc.); measure/decide BT coexistence behavior with HTTPS/networking. Some timeout/abandon items validated in scope but optional captures remain.

## Open questions
- [ ] Priority/order among HTTPS heap fix, regression completion, and BT coexistence; acceptable mitigations (BT disable/release strategy, request sequencing/policy, mbedTLS/ESP-IDF config) while keeping verification enabled?
- [ ] Exact reproduction conditions and “done” criteria for `-141` (clean boot, BT state, endpoints, heap thresholds) and required measurements?
- [ ] Which remaining P7/regression cases are required now and on what hardware/network setup?
- [ ] BT coexistence product decision: supported concurrently or with runtime constraints/UX?

## Validation plan
- Automated: `make format-check`, `make test`, firmware build/lint as applicable.
- Hardware/manual: required (network/HTTPS/TLS, heap profiling, BT scenarios).

## Progress / evidence
- 2026-11-15: created from open R2/R8 follow-ups (investigation/decision/hardware dependent).
