# R24: @cos/http practical enablement pack (post-bounds lock)

- **ID:** r24-http-practical-enablement-pack
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/ready/r24-berry-script-http-server-api.md (v1 simple chat, separate user server); docs/plan/ready/http-upload-measure-and-lock.md; docs/plan/needs-clarification/http-upload-memory-bounds.md; docs/plan/consolidated-improvement-plan.md
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
After http-upload memory bounds are locked/recorded and R24 v1 lands, provide practical enablement: reference examples, operational guidance (limits/timeouts/errors/security), and optionally a narrowly scoped static-file helper only if feasible. Goal: move @cos/http from minimal demo scope to a usable, documented path for small LAN tools on ESP32.

## Scope
- Define example set (e.g. JSON REST-ish endpoints, chat variant refinements) using R24 v1 API within documented limits.
- Write operational guide content (timeouts, 503/504, single-request serialization, body/size limits, security/no-auth warnings, cleanup behavior) suitable for docs.
- Evaluate optional tiny static-file helper (bounded paths, size/mime constraints, memory impact) as a separately approved sub-scope or explicitly out of scope.

## Non-goals
- Auth/TLS, streaming, WebSocket integration inside R24, or async handler redesign beyond R18 review notes.
- Replacing core HTTP server behavior or changing R24 v1 contract except clarifications approved separately.
- Unbounded static serving or arbitrary directory exposure.

## Acceptance criteria
- [ ] Prerequisites stated and enforced: http-upload bounds lock recorded and R24 v1 implementation status acknowledged before implementation work proceeds.
- [ ] At least 2 reference examples specified (and added only after approval/implementation plan) with expected behavior.
- [ ] Operational guide outline approved (topics list) covering limits, errors, timeouts, security, and lifecycle/cleanup.
- [ ] If static-file helper included: strict bounds, security constraints, and validation plan documented; otherwise explicitly deferred.

## Validation plan
- Automated: per AGENTS.md for affected scope (frontend SDK/docs, firmware examples/tests as applicable); make format-check/test/build/lint/docs-check as relevant.
- Hardware/manual: validate examples on-device (Wi-Fi/LAN) when R24 v1 is available; record steps/results and blockers if unavailable.

## Expected areas/files
- storage/ts/ and storage/flash_data/ examples (if added), frontend/src/cos-sdk/http.d.ts/docs, components/http_server or Berry bindings as needed, docs/reports or docs pages for guide content.

## Dependencies / prerequisites
- docs/plan/ready/r24-berry-script-http-server-api.md (v1) implemented/landed or in-flight with clear coupling.
- docs/plan/ready/http-upload-measure-and-lock.md completed and docs/plan/needs-clarification/http-upload-memory-bounds.md locked contract recorded (per R24 constraints).
- R18 async/await follow-up awareness (do not assume async handlers in v1).

## Risks / constraints
- ESP32 memory/stack limits constrain examples and any static serving.
- Security: LAN exposure without auth requires prominent warnings.
- Scope creep risk (keep pack small and bounded).

## Open questions
- [ ] Which two (or more) example scenarios are highest value (chat variants, JSON CRUD mock, sensor endpoint, etc.)?
- [ ] Is a static-file helper in v1 scope of this pack or a separate item?
- [ ] Where should the operational guide live (docs/reports vs user-facing docs vs in-file comments/examples)?

## Progress / evidence
- 2026-11-18: created from analysis of ready/done features (proposed as new feature idea 3).
