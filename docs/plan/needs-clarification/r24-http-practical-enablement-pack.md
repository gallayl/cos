# R24: @cos/http practical enablement pack (post-bounds lock)

- **ID:** r24-http-practical-enablement-pack
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/ready/r24-berry-script-http-server-api.md (v1 simple chat, separate user server); docs/plan/ready/http-upload-measure-and-lock.md; docs/plan/needs-clarification/http-upload-memory-bounds.md; docs/plan/consolidated-improvement-plan.md
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Practical enablement pack for `@cos/http` (R24 v1) delivered **after** http-upload bounds are locked/recorded and R24 v1 is implemented/landed (or clearly in-flight with agreed coupling): reference examples, an operational guide, and **no** static-file helper in this pack. Goal: make R24 v1 usable/documented for small LAN tools on ESP32 without scope creep.

## Scope (decisions recorded 2026-11-18)
- **Examples (v1 of this pack, 1A):** include **two** worked examples using R24 v1 API within documented limits: (a) **JSON API echo+CRUD-mock** (in-memory list/create style endpoints with basic validation/error responses per R24 patterns), and (b) **chat refinements** (build on R24 simple chat concept: bounded store, clear JSON contract). No third example required unless separately approved.
- **Static-file helper:** **out of scope** for this pack (2A). Any future helper must be a separate item with strict bounds/security and explicit approval.
- **Operational guide location (3A):** publish as a **user-facing docs page** (under `docs/`) with links from `docs/plan/ready/r24-berry-script-http-server-api.md` and this item; do not rely solely on plan-folder reports for discoverability (reports may still be used for evidence if helpful).
- **Guide topics (minimum, expected):** limits (body/headers/routes/timeouts/single-request), behavior (503 when busy, 504/timeout, 500 on handler error, double-send/no-response notes), lifecycle/cleanup (close, app/VM teardown expectations), security warnings (LAN exposure, no auth/TLS in v1, input validation responsibility), and practical patterns aligned to R24 v1.
- **Timing/prerequisite gate (4A):** enforce that implementation work for this pack does **not** proceed until (a) `docs/plan/ready/http-upload-measure-and-lock.md` M1 evidence/locked contract is recorded and `docs/plan/needs-clarification/http-upload-memory-bounds.md` locked contract is reflected as required, and (b) R24 v1 (`docs/plan/ready/r24-berry-script-http-server-api.md`) is implemented/landed or implementation is clearly in-flight with agreed coupling. Documentation drafting structure may be prepared earlier only if it does not imply unverified runtime behavior; hardware/example validation happens only after prerequisites met.

## Non-goals
- Auth/TLS, streaming, WebSocket integration inside R24, or async handler redesign beyond R18 review notes.
- Changing R24 v1 contract except clarifications approved separately.
- Static-file serving/helper in this pack; unbounded serving or arbitrary directory exposure in any future work.

## Acceptance criteria
- [ ] Prerequisites enforced and recorded before implementation: http-upload bounds lock recorded (M1 + locked contract) and R24 v1 status acknowledged (landed or in-flight with coupling). Blockers documented if not met.
- [ ] Two reference examples specified with expected behavior (JSON CRUD-mock + chat refinements) and added only after approval/implementation plan and prerequisites satisfied.
- [ ] Operational guide (user-facing docs page) published/updated with agreed topics (limits, 503/504/500/timeout/double-send notes, lifecycle/cleanup, security warnings, practical patterns) and linked from R24 plan item and this item.
- [ ] No static-file helper included; stated as out of scope/deferred.
- [ ] Automated/hardware validation follows `AGENTS.md` and is performed only when prerequisites and R24 v1 availability allow; results recorded or blockers noted.

## Validation plan
- **Automated:** per `AGENTS.md` for affected scope (frontend SDK/docs, firmware/examples/tests as applicable): `make format-check`, `make test`; `make build`/`make lint`/`make docs-check` as relevant.
- **Hardware/manual:** validate examples on-device (Wi‑Fi/LAN) when R24 v1 is available and prerequisites satisfied; record steps/results and blockers if unavailable.

## Expected areas/files (rough)
- `docs/` (new user-facing operational guide page) and links from `docs/plan/ready/r24-berry-script-http-server-api.md`
- `storage/ts/` and `storage/flash_data/` example sources/artifacts (if added), `frontend/src/cos-sdk/http.d.ts` notes only if needed
- `docs/reports/` only if used for evidence/notes (not primary guide location)

## Dependencies / prerequisites
- `docs/plan/ready/r24-berry-script-http-server-api.md` (v1) implemented/landed or in-flight with clear coupling.
- `docs/plan/ready/http-upload-measure-and-lock.md` M1 completed and `docs/plan/needs-clarification/http-upload-memory-bounds.md` locked contract recorded/reflected per R24 constraints.
- R18 async/await awareness (do not assume async handlers in v1).

## Risks / constraints
- ESP32 memory/stack limits constrain examples and any future static serving (static explicitly out of scope here).
- LAN exposure without auth requires prominent warnings in guide and examples.
- Scope creep risk; keep pack to the two examples + guide + links.

## Open questions / blockers
- **Blockers (must be resolved before implementation):** (a) http-upload M1/locked contract not yet recorded/fulfilled per prerequisites; (b) R24 v1 not implemented/landed (or coupling not agreed). Track status in this item and linked plan docs.
- None of the original scoping questions remain open (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A). New questions may arise when R24 v1 APIs/examples are finalized.

## Progress / evidence
- 2026-11-18: created from analysis of ready/done features (proposed as new feature idea 3).
- 2026-11-18 (clarify-feature): reviewed R24 ready spec and upload-bounds items. User choices: two examples (JSON CRUD-mock + chat refinements), static-file helper out of scope, operational guide as user-facing `docs/` page linked from R24 item, enforce prerequisite gate (upload lock + R24 v1 readiness) before implementation; drafting structure allowed only if non-misleading. Item remains **needs-clarification** due to active blockers until prerequisites satisfied (not implementable now).
