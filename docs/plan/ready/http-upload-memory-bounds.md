# Bound HTTP upload memory usage

- **ID:** http-upload-memory-bounds
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `components/http_server/src/http_upload.c:147-197`, `components/http_server/src/http_multipart.c:7-9`, `components/http_server/src/http_multipart.c:122-136`, `components/http_server/src/http_multipart.c:306-310`.
- **Approval:** approved (user approved 2026-11-16)

## Summary
Multipart upload handling currently allocates large temporary heap buffers per request (read buffer + accumulator, ~10 KB total in `http_multipart_parse`) plus request handling overhead; peak contiguous demand is risky on constrained/fragmented ESP32 heap. This feature tightens upload memory bounds with explicit, documented limits (decided): support **single file uploads only, typical < 64 KB** (config/scripts); reject larger with a clear JSON error. Goal is **lower peak heap and safer behavior under fragmentation** via **reduced buffer sizes when parser remains correct** (tradeoffs documented) and **preflight rejection** when `largest_free_block(MALLOC_CAP_8BIT)` is below a documented threshold (clear JSON error).

## Prerequisite milestone: measure and lock bounds/contract (resolved, 2026-11-16 review)
Before `optional-http-websocket-service` implements upload abort/stop behavior or R24 relies on shared/compatible upload handling, complete and record a **measure-and-lock** step (can be the first milestone/PR of this item): (1) capture current/baseline peak/contiguous heap behavior for supported uploads, (2) choose and document the concrete preflight threshold (value/formula + rationale), (3) choose and document proposed `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` (or justify keeping current sizes) with parser-correctness cases/tests that must pass, and (4) lock the upload rejection/abort/error contract table below. No guessed numbers in this plan; values/contract are locked by that recorded step and then treated as the shared contract.

## Upload rejection/abort contract (locked structure, values/details finalized in measure-and-lock)
Shared/core paths and R24 (when reusing shared parsing/helpers, or any R24-specific body path via equivalent/stricter compatible behavior) must follow this contract:

| Case | Trigger | HTTP status | JSON response (shape/fields) | Cleanup / write guarantee | Owner/path |
|---|---|---|---|---|---|
| Preflight low heap | `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` below locked threshold before accepting/buffering as applicable | 503 (or locked equivalent) | JSON error object with stable `error`/code and human message indicating memory/unavailable/try again (exact field names/values locked in measure-and-lock) | No file opened/written; release any request-owned buffers/state | shared upload preflight / core and R24 callers |
| Oversize / unsupported request | body or upload exceeds locked supported limit (<64 KB single file) or unsupported multipart form per bounds | 413 (or locked 400/431 where more specific and documented) | JSON error object with stable fields stating size/limit/unsupported upload (exact fields locked) | No partial/corrupt file written beyond defined safe handling; delete/rollback any temp/partial target if created before rejection point | shared parser/validation |
| Parse/malformed request | malformed multipart/body/headers/encoding within bounded parsing | 400 (or locked specific 4xx) | JSON error object with stable fields indicating malformed/invalid request | No partial/corrupt file written; cleanup buffers/temp state | shared parser/validation |
| Stop/abort during in-flight upload | core HTTP/WS lifecycle stop/abort or equivalent cancellation while upload active (best-effort safe refusal/defer when feasible; otherwise fail in-flight cleanly) | 503 or locked request-failure status (documented) | JSON error object when a response can be sent (same stable error shape family); if transport already closed, log/record per implementation but cleanup still required | **No partial or corrupt file writes** beyond defined safe handling; abort write, close/remove temp/partial artifacts as applicable, release buffers/resources | core stop handler + shared upload cleanup; R24-specific paths must provide equivalent cleanup/no-partial guarantee and compatible 4xx/5xx+JSON semantics where a response is possible |

Exact status choices for ambiguous cases, JSON field names/values/messages, and whether temp files are used are finalized and recorded in the measure-and-lock step; once locked, dependent plans must not diverge.

## Scope
- **Milestone 1 (measure-and-lock, prerequisite for dependents):** document current upload/multipart memory peak and allocation patterns (before/after baseline), record concrete preflight threshold and buffer-size decisions (or justified keep), record parser-correctness cases/tests, and lock the rejection/abort/error contract table above.
- Implement safer bounds per decisions: reduce `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` if parser remains correct (tradeoffs documented) or justify retained sizes; add preflight rejection when `largest_free_block(MALLOC_CAP_8BIT)` is below the locked threshold, with contract-compliant JSON error.
- Enforce/support only: **single file upload, typical < 64 KB** (config/scripts). Reject larger/unsupported uploads with contract-compliant JSON error and documented limit.
- Preserve upload correctness, error reporting, and path validation behavior; apply locked stop/abort cleanup semantics for in-flight cases where applicable.
- Ensure all allocation failure, parse error, rejection, and abort paths remain leak-free and satisfy no-partial/corrupt-write guarantees.

## Non-goals
- New upload API features unrelated to memory/safety.
- General HTTP server architecture rewrite.
- Changing filesystem semantics except as required by streaming/abort-safe cleanup behavior.
- Guessing threshold/buffer values in this spec (must be measured/locked and recorded).

## Acceptance criteria
- [ ] **Measure-and-lock complete:** baseline/current peak + largest-free-block observations recorded; concrete preflight threshold, buffer sizes/decision, parser-correctness cases/tests, and locked rejection/abort/error contract table recorded (prerequisite for dependent core stop/R24 work).
- [ ] Upload peak memory requirements are documented and reduced or explicitly bounded per locked decisions (< 64 KB supported single-file uploads).
- [ ] Requests exceeding the locked size/support limits fail with contract-compliant JSON error and no partial/corrupt writes beyond defined safe handling.
- [ ] Preflight rejects with contract-compliant JSON error when largest free 8-bit block is below the locked threshold (behavior documented).
- [ ] Stop/abort during in-flight upload follows locked contract: safe refusal/defer where feasible, otherwise clean failure with cleanup and no partial/corrupt writes; JSON/error behavior compatible when response possible.
- [ ] Large/invalid/fragmented-memory/abort cases fail safely with clear errors and no leaks.
- [ ] Successful uploads continue to work for supported (< 64 KB, single file) requests.
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (including added/updated integration/QEMU multipart edge-case and abort/error-contract coverage as applicable).

## Validation plan
- **Automated (required):** per `AGENTS.md` / affected scope (`make format-check`, `make test`; `make build` and integration tests/QEMU as affected). Add/extend integration/QEMU multipart edge-case tests for oversize/rejection and error paths where feasible.
- **Hardware/manual (required when hardware/environment available):** spot checks on device for a supported upload, oversize rejection, and near-OOM/fragmented behavior; record before/after largest-free-heap observations (brief note in PR/docs). If unavailable, record concrete blockers and unverified items per `AGENTS.md` (do not claim pass).

## Expected areas/files
- `components/http_server/src/http_multipart.c`, `components/http_server/src/http_multipart.h`
- `components/http_server/src/http_upload.c`
- HTTP server/API docs and tests as applicable

## Dependencies / prerequisites
- **Implementation order (resolved, 2026-11-16 review): hard prerequisite for HTTP/WS lifecycle stop/abort and shared user-server upload behavior.** Complete/record the **measure-and-lock** milestone and lock the upload rejection/abort/error contract table before [`./optional-http-websocket-service.md`](./optional-http-websocket-service.md) implements upload abort/stop behavior and before R24 relies on shared/compatible upload handling. Core HTTP/WS best-effort stop/drain/abort must reuse these locked limits/errors/cleanup semantics and must not introduce partial/corrupt writes beyond defined safe handling.
- **Milestone sequence (HTTP chain):** (1) `http-upload-memory-bounds` measure-and-lock + contract/tests, (2) `optional-http-websocket-service` core lifecycle/abort using locked contract, (3) `r24-berry-script-http-server-api` user server reusing/matching locked contract.
- **R24 (`@cos/http` user server, resolved 2026-11-16 review):** where R24 reuses shared multipart/body/upload parsing or helpers from `components/http_server`, it must depend on and reuse this plan's locked bounds and JSON rejection/error/cleanup contract (no divergent or looser upload handling). If R24 uses a separate request-body path, that path must still document/apply equivalent or stricter bounds and compatible 4xx/413/431/5xx + JSON error semantics and no-partial-write cleanup (no bypass of memory-safety constraints).
- Consider existing HTTP body/response limits and memory-pressure follow-ups in [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md).

## Risks / constraints
- Multipart parsing correctness is easy to regress when buffer strategy changes.
- Smaller buffers may reduce throughput or require more complex state handling.
- Device heap fragmentation can make peak contiguous allocation the limiting factor.

## Open questions
- None (decisions recorded 2026-11-16).

## Decisions / resolved questions
- [x] What upload size/request patterns must be supported on device? **A (decided 2026-11-16):** single file upload, typical < 64 KB (config/scripts). Reject larger with a clear JSON error and documented limit.
- [x] Is the primary goal lower peak heap, better behavior under fragmentation, or both? **C (decided 2026-11-16):** both lower peak heap and safer behavior under fragmentation, with explicit bounds.
- [x] Can buffer sizes be reduced safely with current parser/state machine, or is a larger parser change needed? **A (decided 2026-11-16):** prefer reducing `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` if parser remains correct; document tradeoffs (throughput/complexity). Larger parser redesign only if reduction breaks correctness.
- [x] Should uploads be rejected when free/largest heap is below a threshold, with a specific error? **B (decided 2026-11-16):** reject early if `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` (or equivalent) is below a defined threshold; return a clear error (JSON) and document the threshold (tunable/justified).
- [x] What evidence/benchmarks are required before changing limits? **B (decided 2026-11-16):** add automated integration/QEMU coverage for multipart edge cases plus hardware spot checks (before/after largest-free-heap during a supported upload and behavior near OOM/fragmented case, manual note).

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify round): recorded decisions A/C/A/B/B for Q1–Q5 above.
- 2026-11-16: explicitly approved in chat and marked ready in `docs/plan/ready/` (metadata aligned with folder state). No implementation/validation yet.
- 2026-11-16 (review follow-up): documented implementation order/dependency — **upload bounds first/shared contract** for core HTTP/WS stop and R24 shared upload paths; hardware validation wording clarified (required when available, otherwise blocker).
- 2026-11-16 (review follow-up 2): added explicit measure-and-lock prerequisite milestone and locked upload rejection/abort contract table structure (status/JSON-shape/cleanup/owner cases) as shared contract for core stop and R24; concrete threshold/buffer values and exact JSON fields to be measured/locked and recorded before dependents rely on them. No implementation/validation yet.
