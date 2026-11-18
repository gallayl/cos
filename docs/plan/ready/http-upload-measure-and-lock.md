# Measure and lock HTTP upload memory bounds (M1)

- **ID:** http-upload-measure-and-lock
- **Status:** ready (approved)
- **Created:** 2026-11-17
- **Source/links:** Split from [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md) (approved bounds spec; implementation blocked pending recorded M1 lock). Consolidated plan: [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (HTTP upload/core stop/R24 chain). Audit/context: `components/http_server/src/http_multipart.c` (READ/ACCUM buffers), `components/http_server/src/http_upload.c`.
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-17 ("Approve M1 ready after spec update"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
Deliver the **measure-and-lock (M1)** instrumentation, coverage, procedure, and evidence-reporting needed to unblock [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md) and dependents (`optional-http-websocket-service` upload abort/stop, R24 shared/compatible upload handling). This item is implementable now as code/docs/test/procedure work: add/adjust measurement support and parser-correctness checks as needed, define capture steps, and create/maintain a dated report for results. Actual hardware/QEMU capture runs must be executed and recorded when environments are available (if unavailable, record concrete blockers and unverified items per `AGENTS.md`); parent bounds-implementation remains blocked until the locked results are recorded and cross-linked.

## Scope
- Add or refine instrumentation/hooks/logging/helpers (as appropriate, minimal/resource-conscious) to observe baseline/current upload behavior: peak/typical heap pressure and `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` (and related metrics as useful) for supported single-file uploads (< 64 KB, config/scripts), oversize rejection, malformed/parse cases, and preflight/abort/near-OOM or fragmented scenarios where feasible.
- Define and implement/extend parser-correctness cases/tests needed to safely reduce or justify keeping `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` in `components/http_server/src/http_multipart.c`; document tradeoffs (throughput/complexity).
- Create a canonical dated evidence report under `docs/reports/` (filename: `docs/reports/YYYY-MM-DD-http-upload-measure-and-lock.md`, created/updated during implementation/capture) with: baseline observations, preflight threshold decision (value/formula + rationale), buffer-size decision, parser-correctness evidence, and a **proposed then locked** rejection/abort/error contract table for preflight low heap, oversize/unsupported, parse/malformed, and stop/abort during in-flight upload (exact HTTP statuses, stable JSON field names/values/messages, cleanup/no-partial-write and temp/partial behavior, owner/path). M1 proposes concrete schema/status details for approval alongside measurements.
- Record capture procedure and environment split: **Hardware + QEMU** (user decision). Specify which cases are covered by integration/QEMU vs require hardware (e.g. fragmentation/largest-block pressure) and report blockers/unverified items explicitly when an environment is unavailable.
- Update cross-links/summaries in this item and [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md) (and note dependent plans as needed) so the recorded lock can unblock the HTTP chain once results are approved/locked. Moving the parent item to `ready` is a follow-up only after M1 results are recorded and no blockers/open questions remain.

## Non-goals
- Implementing the full bounded-upload behavior/enforcement changes specified in `http-upload-memory-bounds.md` except minimal, directly necessary changes to support measurement/lock evidence (the enforcement/bounds implementation remains in the parent item, blocked on recorded lock).
- New upload features, general HTTP server architecture rewrite, or unrelated filesystem semantic changes.
- Guessing or hardcoding threshold/buffer/contract values without recorded evidence.
- Claiming hardware/QEMU results that were not executed.

## Acceptance criteria / definition of done
- [ ] Measurement/instrumentation and a documented capture procedure exist for required scenarios (supported <64 KB single-file upload, oversize rejection, malformed parse, preflight low-heap path, stop/abort as applicable, near-OOM/fragmented behavior where feasible), with clear Hardware vs QEMU coverage and known limitations.
- [ ] Parser-correctness cases/tests needed for buffer-size decisions are added/identified and runnable in the appropriate scope; tradeoffs documented.
- [ ] Dated report `docs/reports/YYYY-MM-DD-http-upload-measure-and-lock.md` is created with sections/placeholders for baseline/current peak + largest-free-block observations, chosen preflight threshold (value/formula + rationale), `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` decision (reduce or justify keep), and the proposed/lockable rejection/abort/error contract table (exact statuses, JSON field names/values/messages, cleanup/no-partial-write, temp/partial behavior, owner/path).
- [ ] Cross-links/summaries updated between this item, the report, and [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md); dependents noted as blocked until lock recorded.
- [ ] Any executed captures (QEMU/integration and/or hardware) are recorded with before/after observations and exact steps/commands; unavailable captures are recorded as concrete blockers/unverified items (no false pass claims).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass and are reported with evidence (`make format-check`, `make test`; `make build`/`make lint` when firmware/C/build scope and ESP-IDF env available; `make docs-check` if docs/public headers affected; integration/QEMU tests/build/run as affected/feasible).

## Validation plan
- **Automated (required per affected scope):** run and report `make format-check`, `make test`. If firmware/components or integration tests change: run `make build` and `make lint` when ESP-IDF v6.1/clang-tidy env is available, and add/extend/run integration/QEMU multipart edge/error cases where feasible (`make integration-test-build` / `make integration-test` or `make integration-test-run` as applicable). Run `make docs-check` if Doxygen/public headers/docs checks in scope. Record exact commands, pass/fail, and fixes.
- **QEMU/integration (required when environment available):** execute feasible multipart/upload cases (supported, oversize, malformed, contract/error paths) and record observations/results in the dated report. Document cases not feasible in QEMU.
- **Hardware/manual (required when hardware/environment available):** perform on-device spot checks for supported upload, oversize rejection, near-OOM/fragmented behavior, and relevant abort/stop scenarios; capture before/after largest-free-heap observations and log evidence in the dated report. If hardware unavailable, record concrete blockers and unverified items per `AGENTS.md` (do not claim pass).

## Expected areas/files
- `components/http_server/src/http_multipart.c`, `components/http_server/src/http_multipart.h`, `components/http_server/src/http_upload.c` (instrumentation/hooks and/or correctness adjustments as needed for measurement/lock)
- `integration_test/` (multipart/upload edge/error cases where feasible)
- `test/` or component-level tests where applicable
- `docs/reports/YYYY-MM-DD-http-upload-measure-and-lock.md` (canonical evidence/report)
- `docs/plan/ready/http-upload-measure-and-lock.md`, `docs/plan/needs-clarification/http-upload-memory-bounds.md`, and related plan/cross-link updates as needed

## Dependencies / prerequisites
- Parent consumer: [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md) (defines required lock contents/contract structure).
- Downstream dependents blocked until lock recorded: `optional-http-websocket-service` upload abort/stop, `r24-berry-script-http-server-api` shared/compatible upload handling (see consolidated plan HTTP chain).
- Environment access for **Hardware + QEMU** captures (user decision 2026-11-17). Implementation of instrumentation/procedure is not blocked by lack of immediate captures, but final lock evidence requires captures when available.

## Risks / constraints
- ESP32 heap fragmentation/largest-block behavior is hardware-sensitive; QEMU may not reproduce all cases.
- Multipart parser changes for measurement or buffer experiments can regress correctness; must be guarded by explicit cases/tests.
- Resource-constrained target: instrumentation must be minimal and avoid distorting measurements where possible (document method).
- Locking incorrect values without sufficient evidence would mislead dependents; unverified cases must be called out.

## Decisions / resolved questions
- [x] 2026-11-17 (user): Split M1 measure-and-lock into this separate item.
- [x] 2026-11-17 (user): Evidence environments = **Hardware + QEMU**.
- [x] 2026-11-17 (user): M1 proposes exact JSON error schema/status details for approval with measurements.
- [x] 2026-11-17 (user): Ready scope = implementable now as code/instrumentation/tests/procedure/report work; capture and record Hardware+QEMU results when environments are available (blockers/unverified items recorded if unavailable). Parent remains blocked until results recorded/locked.
- [x] 2026-11-17 (user): Record evidence in a dated `docs/reports/` report plus cross-links/summaries in plan items.
- [x] 2026-11-17 (user): Explicitly approved moving this M1 item to `ready` after spec update (pre-approved for implementation/PR flow; merge remains human).

## Progress / evidence
- 2026-11-17: drafted during `clarify-feature` from split of `http-upload-memory-bounds.md`; user decisions recorded (split, Hardware+QEMU, M1 proposes schema, implement-now scope with captures when available, report+plan cross-links).
- 2026-11-17: specification completed per Ready contract and explicitly approved by user; moved to `docs/plan/ready/http-upload-measure-and-lock.md`. No implementation, measurements, or validation performed yet.
