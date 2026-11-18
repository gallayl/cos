# Measure and lock HTTP upload memory bounds (M1)

- **ID:** http-upload-measure-and-lock
- **Status:** done (delivered 2026-11-18; hardware captures recorded partial and M1 lock recorded/unblocked per report; QEMU and some stress/abort-mechanism gaps remain for follow-up validation)
- **Created:** 2026-11-17
- **Source/links:** Split from [`../needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md) (parent now unblocked/ready: [`../ready/http-upload-memory-bounds.md`](../ready/http-upload-memory-bounds.md)). Consolidated plan: [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (HTTP upload/core stop/R24 chain). Canonical M1 report/procedure/locked contract: [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md). Audit/context: `components/http_server/src/http_multipart.c` (READ/ACCUM buffers), `components/http_server/src/http_upload.c`.
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-17; user requested recording evidence/lock and unblocking dependent features on 2026-11-18 (merge/review remain human).
- **Open questions:** none (remaining QEMU/stress gaps recorded as follow-up validation, not spec blockers)

## Summary
Delivered the **measure-and-lock (M1)** instrumentation, coverage, procedure, parser-correctness host evidence, and dated report needed to unblock [`../ready/http-upload-memory-bounds.md`](../ready/http-upload-memory-bounds.md) and dependents (`optional-http-websocket-service` upload abort/stop, R24 shared/compatible upload handling). Hardware captures were executed and recorded (partial, SD-path) and the preflight threshold/buffer decision/rejection-abort-error contract were **locked** in [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md). QEMU endpoint captures and some forced fragmentation/core stop-abort mechanism cases remain follow-up validation gaps; enforcement/cleanup behavior is implemented in parent/dependent items per locked contract.

## Scope
- [x] Add/refine minimal instrumentation to observe upload behavior: peak/typical heap pressure and `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` for supported single-file uploads (< 64 KB), oversize/malformed/parse cases, and abort/receive-failure behavior where feasible (`http_multipart.c`, `http_upload.c`).
- [x] Define and extend parser-correctness cases/tests needed to justify keeping `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` in `components/http_server/src/http_multipart.c`; document tradeoffs.
- [x] Create/maintain canonical dated report [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md) with baseline/hardware observations, locked preflight threshold (12 KiB `largest_8bit`, evidence-based), buffer-size decision (keep 2048/8192), parser-correctness evidence, and locked rejection/abort/error contract table (statuses, JSON fields/messages, cleanup/no-partial-write, owner/path).
- [x] Record capture procedure and environment split: **Hardware + QEMU**. Hardware captures executed/recorded (partial, SD path, COM3/STA 192.168.0.187); QEMU endpoint captures not executed (gap recorded).
- [x] Update cross-links/summaries in this item/report and parent/dependent plans so recorded lock unblocks the HTTP chain. Parent moved to `ready` with locked contract referenced.

## Non-goals
- Implementing the full bounded-upload behavior/enforcement changes specified in `http-upload-memory-bounds.md` except minimal, directly necessary changes to support measurement/lock evidence (the enforcement/bounds implementation remains in the parent item, blocked on recorded lock).
- New upload features, general HTTP server architecture rewrite, or unrelated filesystem semantic changes.
- Guessing or hardcoding threshold/buffer/contract values without recorded evidence.
- Claiming hardware/QEMU results that were not executed.

## Acceptance criteria / definition of done
- [x] Measurement/instrumentation and a documented capture procedure exist for required scenarios (supported <64 KB single-file upload, oversize rejection, malformed parse, preflight low-heap path, stop/abort as applicable, near-OOM/fragmented behavior where feasible), with clear Hardware vs QEMU coverage and known limitations.
- [x] Parser-correctness cases/tests needed for buffer-size decisions are added/identified and runnable in the appropriate scope; tradeoffs documented (host parse cases extended in `test/test_http_multipart.c`, mock recv support in `test/mocks/mock_httpd.{c,h}`; additional multi-recv/boundary-split cases identified).
- [x] Dated report `docs/reports/2026-11-18-http-upload-measure-and-lock.md` contains baseline/hardware observations, **locked** preflight threshold (12 KiB `largest_8bit`, evidence-based), `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` decision (keep 2048/8192), parser-correctness evidence, and **locked** rejection/abort/error contract table (statuses, JSON fields/messages, cleanup/no-partial-write, owner/path).
- [x] Cross-links/summaries updated between this item, report, parent [`../ready/http-upload-memory-bounds.md`](../ready/http-upload-memory-bounds.md), and dependents; M1 lock recorded/unblocking.
- [x] Executed hardware captures recorded with observations/commands/artifacts (`docs/reports/artifacts/hw-2026-11-18-http-upload-m1-com3/`); unavailable/QEMU/stress gaps recorded as concrete blockers/unverified items (no false pass claims).
- [x] Automated checks required for affected scope reported with evidence: `make format-check` **PASS** (2026-11-18), `make test` **PASS** (CTest 30/30 incl. multipart/upload tests, Python scripts 3/3). `make build`/`make lint` **not run** (ESP-IDF v6.1/clang-tidy env unavailable in sessions), `docs-check` not required for code scope, integration/QEMU not run (harness/env gaps). Hardware/manual validation executed and recorded (partial).

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
- Parent consumer (now unblocked): [`../ready/http-upload-memory-bounds.md`](../ready/http-upload-memory-bounds.md) (bounds enforcement to implement per locked contract).
- Downstream dependents (prerequisite satisfied by recorded lock): `optional-http-websocket-service` upload abort/stop, `r24-berry-script-http-server-api` shared/compatible upload handling (see consolidated plan HTTP chain).
- **Follow-up validation gaps (non-blocking for lock):** QEMU multipart endpoint captures, forced fragmentation/preflight stress, core stop/abort-mechanism behavior, and improved SD file-size/content verification. Flash `/flash` upload path showed device boot-loop incident on tested unit (use SD/validate FS health).

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
- 2026-11-17: drafted/specified, approved, moved to `docs/plan/ready/http-upload-measure-and-lock.md`.
- 2026-11-18 (implementation): M1 instrumentation/procedure/parser-correctness host cases/report delivered on `feat/http-upload-measure-and-lock-m1` (commits `d72a42c`, `a9f10ff`, merged with `origin/main`; PR #78). Automated checks: `make format-check` **PASS**, `make test` **PASS** (CTest 30/30 incl. `test_http_multipart`/`test_http_upload`, Python 3/3). `make build`/`make lint` not run (ESP-IDF env unavailable), integration/QEMU not run.
- 2026-11-18 (hardware capture + lock): executed on-device validation (COM3, ESP32 `cos` `f2a8d7d`, STA `192.168.0.187`, auth `cos:cos123`, SD `/sdcard/m1tmp`) and recorded in [`../../reports/2026-11-18-http-upload-measure-and-lock.md`](../../reports/2026-11-18-http-upload-measure-and-lock.md) with artifacts `docs/reports/artifacts/hw-2026-11-18-http-upload-m1-com3/`: successful 1/16/63 KB uploads with `upload-measure` heap/largest_8bit phases, 80 KB accepted (current behavior), malformed/missing-path cases, client-abort recv/parse failure with possible partial artifact; flash `/flash` probe caused watchdog boot-loop (device reset, switched to SD). **Locked:** preflight threshold **12 KiB `largest_8bit`** (evidence-based), buffers **keep 2048/8192**, rejection/abort/error contract table (statuses/JSON/cleanup/owner). QEMU and forced fragmentation/core stop-abort mechanism gaps remain recorded. Parent moved to ready and dependents unblocked by recorded lock; moved this item to `docs/plan/done/`.
