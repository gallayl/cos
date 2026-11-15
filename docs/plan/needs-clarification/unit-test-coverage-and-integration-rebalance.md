# Unit test coverage and integration rebalancing (R25 #11)

- **ID:** unit-test-coverage-and-integration-rebalance
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](../2026-11-14-unit-test-coverage-and-integration-rebalance.md) (proposed/analysis, has open questions), [`ideas/2026-11-10-refactoring-and-testability.md`](../../ideas/2026-11-10-refactoring-and-testability.md) (R25 #11, #4)
- **Approval:** pending

## Summary
Analysis/proposal to expand host unit coverage (HTTP API handlers/helpers, network core, time sync edges, etc.), add seams where needed, and rebalance deterministic cases from integration to unit tests while keeping platform/runtime coverage in integration/QEMU.

## Open questions
- [ ] Prioritized scope/increments to implement now (which P1/P2 items, in what order) and target coverage/acceptance per increment?
- [ ] Agreed refactor/seam boundaries (especially `http_api.c`/esp_http_server glue, `network_http.c` stateful client, console transport/input per R25 #4) with minimal-risk design?
- [ ] Which integration tests/cases can be safely rebalanced or removed with equivalent host coverage (without losing needed E2E/platform confidence)?
- [ ] CI/runtime budget and environment constraints for added tests?

## Acceptance criteria
- [ ] Selected increments specified and approved as ready items; implemented with tests passing and documented rationale/evidence for any integration rebalancing.

## Validation plan
- Automated: `make format-check`, `make test`, firmware build/lint/integration as affected per `AGENTS.md`.

## Progress / evidence
- 2026-11-15: created from proposed plan/open questions (needs prioritization and design decisions).
