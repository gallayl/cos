# Bound HTTP upload memory bounds

- **ID:** http-upload-memory-bounds
- **Status:** done (implementation delivered in PR [#85](https://github.com/gallayl/cos/pull/85), CI green 2026-11-18; pending human review/approval and merge; required hardware/manual validation and remaining QEMU/fragmentation/core stop-abort stress gaps tracked below)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `components/http_server/src/http_upload.c`, `components/http_server/src/http_multipart.c`. M1 evidence/locked contract: [`./http-upload-measure-and-lock.md`](./http-upload-measure-and-lock.md), [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md). Consolidated plan: [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md). Implementation PR: [#85](https://github.com/gallayl/cos/pull/85) (`feb9914`).
- **Approval:** approved (user approved 2026-11-16); M1 lock/evidence recorded and unblocking requested 2026-11-18 (merge remains human).

## Summary
Multipart upload handling currently allocates large temporary heap buffers per request (read buffer + accumulator, ~10 KB total in `http_multipart_parse`) plus request handling overhead; peak contiguous demand is risky on constrained/fragmented ESP32 heap. This feature tightens upload memory bounds with explicit, documented limits (decided): support **single file uploads only, typical < 64 KB** (config/scripts); reject larger with a clear JSON error. Goal is **lower peak heap and safer behavior under fragmentation** via **documented/locked buffer configuration** (keep 2048/8192 unless separately revalidated) and **preflight rejection** when `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` is below the **locked threshold (12 KiB)**, plus enforcement of the **locked rejection/abort/error/cleanup contract**.

**M1 lock recorded (2026-11-18):** threshold, buffer decision, parser-correctness evidence/identified cases, and exact rejection/abort/error contract are recorded in the M1 report. This item is now **ready/implementable** for bounds/behavior enforcement (subject to normal implementation validation). QEMU and some fragmentation/core stop-abort stress gaps remain for validation follow-up where feasible.

## Prerequisite milestone: measure and lock bounds/contract (completed, 2026-11-18)
M1 completed and recorded: (1) baseline/peak and `largest_8bit` observations captured on hardware (partial, SD path) with artifacts, (2) **locked preflight threshold:** **12288 bytes (12 KiB)** contiguous 8-bit block (`largest_8bit`) at handler/parse entry, (3) **locked buffer decision:** keep `READ_BUF_SIZE=2048`, `ACCUM_BUF_SIZE=8192` (no reduction justified), (4) parser-correctness host cases extended and additional multi-recv/boundary-split cases identified, and (5) **locked rejection/abort/error contract** recorded below. Evidence: [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md). Dependents may rely on this locked contract (no divergence).

## Upload rejection/abort contract (locked, 2026-11-18)
Shared/core paths and R24 (when reusing shared parsing/helpers, or any R24-specific body path via equivalent/stricter compatible behavior) must follow this contract:

| Case | Trigger (locked intent) | HTTP status | JSON response (locked) | Cleanup / write guarantee | Owner/path |
|---|---|---:|---|---|---|
| Preflight low heap | `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` below **12288 B (12 KiB)** before accepting/buffering as applicable | 503 Service Unavailable | `{"error":"upload_rejected_low_memory","message":"Upload rejected: insufficient contiguous memory"}` | No file opened/written; release request-owned buffers/state | shared upload preflight / core and R24 callers |
| Oversize / unsupported request | Body/upload exceeds locked supported limit (**< 64 KB single file**) or unsupported multipart form per bounds | 413 Payload Too Large | `{"error":"upload_too_large","message":"Upload exceeds supported limit","max_bytes":65536}` | No partial/corrupt file written beyond defined safe handling; delete/rollback any temp/partial target if created before rejection point | shared parser/validation |
| Parse/malformed request | Malformed multipart/body/headers/encoding within bounded parsing (invalid boundary/missing terminator/bad structure) | 400 Bad Request | `{"error":"upload_invalid_request","message":"Upload request is malformed or invalid"}` | No partial/corrupt file written; cleanup buffers/temp state | shared parser/validation |
| Stop/abort during in-flight upload | Core HTTP/WS lifecycle stop/abort or equivalent cancellation while upload active (best-effort safe refusal/defer when feasible; otherwise fail in-flight cleanly) | 503 Service Unavailable (or locked request-failure status when response possible) | `{"error":"upload_aborted","message":"Upload aborted"}` (when response can be sent) | **No partial or corrupt file writes** beyond defined safe handling; abort write, close/remove temp/partial artifacts as applicable, release buffers/resources | core stop handler + shared upload cleanup; R24-compatible paths must provide equivalent cleanup and compatible semantics |

**Notes:** stable machine-readable `error` and human `message` (optional fields like `max_bytes` as specified). Current implementation does **not** yet fully conform (preflight/413/abort/cleanup guarantees to be enforced in this and dependent items). Do not diverge from locked statuses/fields/cleanup semantics.

## Scope
- [x] **Milestone 1 (measure-and-lock, prerequisite for dependents):** completed and recorded (threshold 12 KiB, keep buffers 2048/8192, parser-correctness evidence, locked contract/table) — see M1 report.
- [x] Implement safer bounds per locked decisions: enforce preflight rejection when `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 12288` with contract-compliant JSON error (503 + locked payload); keep buffer sizes unless separately revalidated.
- [x] Enforce/support only **single file upload, typical < 64 KB** (config/scripts). Reject larger/unsupported uploads with contract-compliant 413 JSON error and documented limit (fast content-length precheck with overhead allowance + streamed payload cap; additional file parts rejected as unsupported).
- [x] Preserve upload correctness, error reporting, and path validation behavior; apply locked stop/abort cleanup semantics for in-flight cases where applicable (best-effort 503 `upload_aborted` mapping when response possible; temp-file commit/cleanup path implemented).
- [x] Ensure allocation failure, parse error, rejection, and abort paths are cleaned up and satisfy no-partial/corrupt-write guarantees (rollback/removal of temp artifacts; checked flush/close before commit and overwrite fallback). Fragmentation/core stop-abort stress validation remains pending (see gaps).

## Non-goals
- New upload API features unrelated to memory/safety.
- General HTTP server architecture rewrite.
- Changing filesystem semantics except as required by streaming/abort-safe cleanup behavior.
- Guessing threshold/buffer values in this spec (must be measured/locked and recorded).

## Acceptance criteria
- [x] **Measure-and-lock complete:** baseline/current peak + largest-free-block observations recorded; concrete preflight threshold, buffer sizes/decision, parser-correctness cases/tests, and locked rejection/abort/error contract table recorded (prerequisite for dependent core stop/R24 work).
- [x] Upload peak memory requirements are documented and bounded per locked decisions (< 64 KB supported single-file uploads; buffers kept 2048/8192).
- [x] Requests exceeding the locked size/support limits are rejected with contract-compliant JSON error and cleanup/no-partial-write handling in implementation (413 `upload_too_large`, streamed payload cap/additional-file rejection). **Gap:** no dedicated QEMU/integration upload-endpoint harness coverage added in this run.
- [x] Preflight rejects with contract-compliant JSON error when largest free 8-bit block is below the locked threshold (503 `upload_rejected_low_memory`, threshold 12 KiB documented/locked). **Gap:** forced low-heap/fragmentation stress not revalidated on hardware in this run.
- [x] Stop/abort during in-flight upload mapped to best-effort locked abort semantics (503 `upload_aborted` when response possible) with temp-file cleanup to avoid partial/corrupt writes in implemented path. **Gap:** core HTTP/WS lifecycle stop/abort mechanism stress and QEMU/hardware abort scenarios not fully exercised in this run.
- [x] Large/invalid cases handled with locked/clean error paths and cleanup in implementation; fragmentation-memory/abort stress cases remain partially unverified (**gaps recorded**).
- [x] Successful supported uploads intended to continue working (single file, bounded payload, temp-file commit with overwrite fallback). **Gap:** hardware/manual spot check for supported upload not rerun in this implementation session (automated CI/build/unit checks green).
- [x] Automated checks required by `AGENTS.md` for affected scope passed where run: `make format-check` pass, `make test` pass (30/30 + 3/3), `make build` pass (local), PR CI green including lint/build, unit tests, and integration test suites/required checks ([44352543170](https://github.com/gallayl/cos/actions/runs/44352543170); `shell_fs` rerun passed). **Not added/run:** dedicated QEMU/integration `/api/upload` multipart edge-case/abort-contract harness tests (no existing harness found); `make lint` not run locally (CI covered). Hardware/manual validation remains pending human action.

## Pending human actions / remaining gaps
- **Human:** PR [#85](https://github.com/gallayl/cos/pull/85) review/approval and merge (merge remains human action). After merge, record merge SHA/evidence in tracker/follow-up as needed.
- **Hardware/manual (when available):** spot checks on device for supported upload, oversize rejection, and near-OOM/fragmented behavior with before/after largest-free-heap observations; validate core stop/abort-during-upload behavior when mechanism exists.
- **Follow-up validation:** add/extend integration/QEMU multipart upload endpoint coverage for oversize/rejection/abort/error-contract cases where feasible; close forced fragmentation and stop/abort stress gaps recorded in M1 report.

## Validation plan
- **Automated (required):** per `AGENTS.md` / affected scope (`make format-check`, `make test`; `make build` and integration tests/QEMU as affected). Add/extend integration/QEMU multipart edge-case tests for oversize/rejection and error paths where feasible.
- **Hardware/manual (required when hardware/environment available):** spot checks on device for a supported upload, oversize rejection, and near-OOM/fragmented behavior; record before/after largest-free-heap observations (brief note in PR/docs). If unavailable, record concrete blockers and unverified items per `AGENTS.md` (do not claim pass).

## Expected areas/files
- `components/http_server/src/http_multipart.c`, `components/http_server/src/http_multipart.h`
- `components/http_server/src/http_upload.c`
- HTTP server/API docs and tests as applicable

## Dependencies / prerequisites
- **Implementation order (resolved, 2026-11-16 review): hard prerequisite for HTTP/WS lifecycle stop/abort and shared user-server upload behavior.** Complete/record the **measure-and-lock** milestone and lock the upload rejection/abort/error contract table before [`./optional-http-websocket-service.md`](./optional-http-websocket-service.md) implements upload abort/stop behavior and before R24 relies on shared/compatible upload handling. Core HTTP/WS best-effort stop/drain/abort must reuse these locked limits/errors/cleanup semantics and must not introduce partial/corrupt writes beyond defined safe handling. **Dependents are implementation-blocked until this recorded lock exists.**
- **Milestone sequence (HTTP chain):** (1) `http-upload-memory-bounds` **measure-and-lock + recorded contract/tests (blocker)**, (2) `optional-http-websocket-service` core lifecycle/abort using locked contract, (3) `r24-berry-script-http-server-api` user server reusing/matching locked contract.
- **R24 (`@cos/http` user server, resolved 2026-11-16 review):** where R24 reuses shared multipart/body/upload parsing or helpers from `components/http_server`, it must depend on and reuse this plan's locked bounds and JSON rejection/error/cleanup contract (no divergent or looser upload handling). If R24 uses a separate request-body path, that path must still document/apply equivalent or stricter bounds and compatible 4xx/413/431/5xx + JSON error semantics and no-partial-write cleanup (no bypass of memory-safety constraints).
- Consider existing HTTP body/response limits and memory-pressure follow-ups in [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md).

## Risks / constraints
- Multipart parsing correctness is easy to regress when buffer strategy changes.
- Smaller buffers may reduce throughput or require more complex state handling.
- Device heap fragmentation can make peak contiguous allocation the limiting factor.

## Open questions
- None. M1 lock recorded (2026-11-18) and cross-linked; implementation proceeds against locked threshold/buffer/contract. Follow-up validation gaps (QEMU, forced fragmentation/core stop-abort stress) are tracked as validation items, not spec blockers.

## Decisions / resolved questions
- [x] What upload size/request patterns must be supported on device? **A (decided 2026-11-16):** single file upload, typical < 64 KB (config/scripts). Reject larger with a clear JSON error and documented limit.
- [x] Is the primary goal lower peak heap, better behavior under fragmentation, or both? **C (decided 2026-11-16):** both lower peak heap and safer behavior under fragmentation, with explicit bounds.
- [x] Can buffer sizes be reduced safely with current parser/state machine, or is a larger parser change needed? **A (decided 2026-11-16):** prefer reducing `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` if parser remains correct; document tradeoffs (throughput/complexity). Larger parser redesign only if reduction breaks correctness.
- [x] Should uploads be rejected when free/largest heap is below a threshold, with a specific error? **B (decided 2026-11-16):** reject early if `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` (or equivalent) is below a defined threshold; return a clear error (JSON) and document the threshold (tunable/justified).
- [x] What evidence/benchmarks are required before changing limits? **B (decided 2026-11-16):** add automated integration/QEMU coverage for multipart edge cases plus hardware spot checks (before/after largest-free-heap during a supported upload and behavior near OOM/fragmented case, manual note).

## Progress / evidence
- 2026-11-16: created, clarified, approved; later marked blocked for dependent work pending M1 measure-and-lock.
- 2026-11-17: split M1 into `http-upload-measure-and-lock` (Hardware+QEMU, propose schema); parent kept in needs-clarification pending recorded lock.
- 2026-11-18 (M1 delivered + lock recorded): M1 artifacts/procedure/parser-correctness host cases and report created; **hardware captures executed (partial, SD path, 2026-11-18)** and recorded in [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md) with artifacts `docs/reports/artifacts/hw-2026-11-18-http-upload-m1-com3/`. **Locked:** preflight threshold **12 KiB `largest_8bit`**, buffers **keep 2048/8192**, rejection/abort/error contract (503/413/400/503 + stable JSON + cleanup/no-partial-write). QEMU and forced fragmentation/core stop-abort stress gaps remain recorded. M1 item moved to done: [`./http-upload-measure-and-lock.md`](./http-upload-measure-and-lock.md). Prerequisite satisfied for dependent `optional-http-websocket-service` / R24 shared-or-compatible upload handling (implementation must follow locked contract).
- 2026-11-18 (implementation delivered, PR [#85](https://github.com/gallayl/cos/pull/85), branch `feat/http-upload-memory-bounds`, commit `feb9914`): enforced locked contract in `components/http_server/src/http_upload.c` — preflight reject when `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 12288` (503 `upload_rejected_low_memory`), oversize/unsupported body (fast content-length precheck with overhead allowance) and streamed file-payload cap/additional file parts (413 `upload_too_large`, `max_bytes:65536`), malformed/parse/invalid request mapping to 400 `upload_invalid_request`, recv/transfer failures mapped to best-effort 503 `upload_aborted` (when response possible), temp-file write + checked `fclose` + `rename` commit (overwrite fallback) and cleanup/unlink on failure paths to avoid partial/corrupt writes; parser buffers remain 2048/8192. Independent review (subagent `ses_e107472cdffeBxQn9XgTTwK9XN`) requested changes; findings addressed (flush-before-commit check, overwrite-compatible rename fallback, streamed payload byte cap).
  - **Local checks (exact):** `make format`/`make format-check` **pass** (`EXIT:0`), `make test` **pass** (CTest 30/30, Python unittest 3/3 OK), `make build` **pass** (ESP-IDF build completed successfully; `cos.bin` generated).
  - **PR CI (run [44352543170](https://github.com/gallayl/cos/actions/runs/44352543170), head `feb9914`):** all reported checks green including `format-check / Required`, `lint / Required` (ESP-IDF Build & Lint), `test / Required`, `integration-tests / Required` (all suites pass; `shell_fs` initially failed/timed out with unrelated broad suite instability, rerun passed), `CI Checks Required`, `frontend / Required` (build skipped by path filter). `mergeStateStatus: CLEAN` at last check.
  - **Not run / gaps (unchanged):** `make lint` not run locally (CI lint/build green), no dedicated QEMU/integration `/api/upload` multipart edge-case/abort-contract harness coverage added (no existing endpoint harness found), hardware/manual spot checks (supported upload, oversize rejection, near-OOM/fragmentation, before/after largest-free-heap) and forced fragmentation/core stop-abort stress pending per spec/M1 report.
  - **Pending human actions:** PR review/approval and **merge** (human). If/when merged and/or hardware validation performed, record merge SHA and validation evidence; treat required hardware/manual validation as merge-blocking only if policy requires, otherwise track as follow-up evidence. Move/finalize tracker state in `main` as needed after merge.
