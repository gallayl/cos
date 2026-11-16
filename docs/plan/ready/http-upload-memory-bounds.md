# Bound HTTP upload memory usage

- **ID:** http-upload-memory-bounds
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `components/http_server/src/http_upload.c:147-197`, `components/http_server/src/http_multipart.c:7-9`, `components/http_server/src/http_multipart.c:122-136`, `components/http_server/src/http_multipart.c:306-310`.
- **Approval:** approved (user approved 2026-11-16)

## Summary
Multipart upload handling currently allocates large temporary heap buffers per request (read buffer + accumulator, ~10 KB total in `http_multipart_parse`) plus request handling overhead; peak contiguous demand is risky on constrained/fragmented ESP32 heap. This feature tightens upload memory bounds with explicit, documented limits (decided): support **single file uploads only, typical < 64 KB** (config/scripts); reject larger with a clear JSON error. Goal is **lower peak heap and safer behavior under fragmentation** via **reduced buffer sizes when parser remains correct** (tradeoffs documented) and **preflight rejection** when `largest_free_block(MALLOC_CAP_8BIT)` is below a documented threshold (clear JSON error).

## Scope
- Document current upload/multipart memory peak and allocation patterns (before/after) and the enforced limits.
- Implement safer bounds per decisions: reduce `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` if parser remains correct (document tradeoffs); add preflight rejection when `largest_free_block(MALLOC_CAP_8BIT)` is below a documented threshold, with a clear JSON error.
- Enforce/support only: **single file upload, typical < 64 KB** (config/scripts). Reject larger uploads with a clear JSON error and documented limit.
- Preserve upload correctness, error reporting, and path validation behavior.
- Ensure all allocation failure and parse error paths remain leak-free.

## Non-goals
- New upload API features unrelated to memory/safety.
- General HTTP server architecture rewrite.
- Changing filesystem semantics except as required by streaming behavior.

## Acceptance criteria
- [ ] Upload peak memory requirements are documented and reduced or explicitly bounded (target reflects decisions: lower peak + fragmentation-safe behavior, < 64 KB supported single-file uploads).
- [ ] Requests exceeding the documented size limit fail with a clear JSON error (and no partial/corrupt writes beyond current safe behavior).
- [ ] Preflight rejects with a clear JSON error when largest free 8-bit block is below the documented threshold (behavior documented).
- [ ] Large/invalid/fragmented-memory cases fail safely with clear errors and no leaks.
- [ ] Successful uploads continue to work for supported (< 64 KB, single file) requests.
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (including added/updated integration/QEMU multipart edge-case coverage as applicable).

## Validation plan
- **Automated (required):** per `AGENTS.md` / affected scope (`make format-check`, `make test`; `make build` and integration tests/QEMU as affected). Add/extend integration/QEMU multipart edge-case tests for oversize/rejection and error paths where feasible.
- **Hardware/manual (required when hardware/environment available):** spot checks on device for a supported upload, oversize rejection, and near-OOM/fragmented behavior; record before/after largest-free-heap observations (brief note in PR/docs). If unavailable, record concrete blockers and unverified items per `AGENTS.md` (do not claim pass).

## Expected areas/files
- `components/http_server/src/http_multipart.c`, `components/http_server/src/http_multipart.h`
- `components/http_server/src/http_upload.c`
- HTTP server/API docs and tests as applicable

## Dependencies / prerequisites
- **Implementation order (resolved, 2026-11-16 review): hard prerequisite for HTTP/WS lifecycle stop/abort and shared user-server upload behavior.** This plan must be implemented and its upload size/patterns, buffer/preflight bounds, and JSON rejection/error/cleanup contract must be locked before [`./optional-http-websocket-service.md`](./optional-http-websocket-service.md) implements upload abort/stop behavior. Core HTTP/WS best-effort stop/drain/abort must reuse these limits/errors/semantics and must not introduce partial/corrupt writes beyond defined safe handling.
- **R24 (`@cos/http` user server, resolved 2026-11-16 review):** where R24 reuses shared multipart/body/upload parsing or helpers from `components/http_server`, it must depend on and reuse this plan's bounded limits and JSON error/cleanup contract (no divergent or looser upload handling). If R24 uses a separate request-body path, that path must still document/apply equivalent or stricter bounds and compatible 4xx/413/431/error semantics (no bypass of memory-safety constraints).
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
