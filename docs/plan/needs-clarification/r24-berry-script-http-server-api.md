# R24 — Berry/TS callback-based HTTP server API (Express-like)

- **ID:** r24-berry-script-http-server-api
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (R24), `components/http_server/*`, `components/berry_lang/src/berry_network.cpp`, `frontend/src/cos-sdk/network.d.ts`
- **Approval:** pending

## Summary
Expose a script-facing HTTP server API (routing/callbacks, request/response helpers) for Berry/TS built on existing HTTP server/ESP-IDF, with safe C↔Berry callback bridging and resource constraints respected.

## Open questions
- [ ] Approved API design/surface (`@cos/http` vs other module, routing/middleware scope, request/response fields/methods, body types/limits, concurrency model)?
- [ ] Threading/task context and Berry callback safety/lifetime rules; error handling and backpressure?
- [ ] Security posture (LAN-only assumptions, auth/exposure guidance) and acceptable defaults?
- [ ] Minimal milestone for “ready” implementation and required examples/tests/hardware validation?

## Acceptance criteria
- [ ] Clarified/approved design spec; implementation delivers scoped API + SDK types + examples + tests + evidence per spec.

## Validation plan
- Automated: `make format-check`, `make test`, firmware build/lint, frontend checks if SDK affected.
- Hardware/manual: required for on-device server behavior/network validation.

## Progress / evidence
- 2026-11-15: created from ideas (design/scope decisions required).
