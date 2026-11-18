# HTTP upload measure-and-lock (M1) report

- **Item:** [`docs/plan/ready/http-upload-measure-and-lock.md`](../plan/ready/http-upload-measure-and-lock.md)
- **Parent (blocked on recorded lock):** [`docs/plan/needs-clarification/http-upload-memory-bounds.md`](../plan/needs-clarification/http-upload-memory-bounds.md)
- **Date:** 2026-11-18
- **Status:** M1 instrumentation/procedure/report/proposed contract prepared; **no Hardware/QEMU capture runs executed in this session** (lock pending recorded evidence)

## Purpose

Provide measurement instrumentation, capture procedure, parser-correctness evidence/identified cases, and a **proposed** (to be locked with measurements) preflight threshold, buffer-size decision, and upload rejection/abort/error contract for supported single-file uploads (< 64 KB, config/scripts). This report is the canonical evidence location for M1; actual captures must be appended when Hardware/QEMU environments are available.

## Instrumentation and measurement method

### Code instrumentation (added for M1)

- **`components/http_server/src/http_multipart.c`**: logs heap snapshots with tag `http_multipart` prefix `upload-measure:` at parser start (content length, buffer sizes), after buffer allocation, on allocation/accumulator overflow/receive/parse-state failures, and on completion. Reports `free` (`esp_get_free_heap_size()`), `largest_8bit` (`heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)`), `content_len`, `remaining`, `accum_len`, and buffer configuration as applicable.
- **`components/http_server/src/http_upload.c`**: logs heap snapshots with tag `http_upload` prefix `upload-measure:` at handler entry (content length), after multipart parse returns (result, error string if set), and final handler outcome. Existing functional behavior/error mapping is preserved except added observability logs.

**Method notes:** `ESP_LOGI` is used for capturable procedure evidence; logging adds small, transient stack/heap/latency overhead and can perturb fragmentation-sensitive measurements. For lock decisions, prefer comparing relative trends and capture both baseline and candidate configurations with identical logging/build settings. Record build ID/commit, sdkconfig/profile, and enabled services/boot profile for every capture.

### Buffer configuration (current, pre-lock)

From `components/http_server/src/http_multipart.c`:

- `READ_BUF_SIZE = 2048`
- `ACCUM_BUF_SIZE = READ_BUF_SIZE * 4 = 8192`
- Per-request parser allocations during parse: `buf` (`READ_BUF_SIZE + 1`) and `accum` (`ACCUM_BUF_SIZE`) via heap `malloc` (~10 KB total, plus request/stack/filesystem overhead).

No buffer-size reduction or preflight enforcement is implemented in M1 (non-goal); decision is proposed/justified below pending measurements.

## Capture procedure (Hardware + QEMU)

**Environment split (required):** execute feasible cases in **QEMU/integration** and **hardware/manual**. Record environment, commit, build target/profile, network/FS state, and whether core HTTP service is enabled.

### Common preparation

1. Build/flash/run firmware containing M1 instrumentation (or QEMU image for integration cases).
2. Enable log capture including `http_upload` and `http_multipart` info logs; filter/grep for `upload-measure:`.
3. Before each scenario, record idle baseline: `free` heap and `largest_8bit` (`heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)`), plus any relevant `svc_mem`/service diagnostics snapshot if available.
4. For each upload attempt record: scenario name, request/Content-Length, file size/path, observed HTTP status + JSON body, filesystem outcome (file created/absent/unchanged, no partial/corrupt target where applicable), and all `upload-measure` log lines.

### Required scenarios

| Scenario | Purpose | QEMU/integration | Hardware/manual |
|---|---|---|---|
| Supported single-file upload (< 64 KB, e.g. small config/script ~1 KB, ~16 KB, ~63 KB) | Baseline/typical peak and correctness | Feasible when multipart upload endpoint exercisable in QEMU harness; otherwise document gap | **Required when available** |
| Oversize/unsupported upload (e.g. > 64 KB single file, or unsupported form as defined by bounds spec) | Observe current behavior and capture contract evidence | Feasible if request path reachable; current implementation may stream/accept beyond target bound (record actual) | **Required when available** |
| Malformed/parse cases (bad boundary, truncated multipart, missing terminator, invalid Content-Type/boundary) | Parser behavior and cleanup observations | Feasible with crafted requests where harness supports it | Recommended/required when available |
| Preflight low-heap path (simulated/constrained heap or fragmented state, if feasible) | Determine safe `largest_8bit` threshold | Usually **not feasible** to reproduce fragmentation accurately | **Required when available** if reproducible; otherwise record limitation/blocker |
| Stop/abort during in-flight upload | Cleanup/no-partial-write behavior and error contract feasibility | Limited feasibility; document if unsupported in harness | **Required when available** when stop/abort mechanism exists and safe to test |
| Near-OOM/fragmented behavior | Largest-block pressure and failure modes | Limited; QEMU heap model may differ | **Required when available** (primary evidence for fragmentation) |

### Suggested request shapes (adapt to actual endpoint/auth)

- Endpoint: `POST /api/upload` (multipart/form-data), authenticated per server config.
- Form fields: `path` (target, e.g. `/flash/upload-test.txt` or `/sdcard/...` per policy) and file part (field name as accepted by server, typically `file`/file part; verify with implementation).
- Capture exact `curl`/client command, headers, and payload generation (including boundary) in results.

### Results template (append per capture)

For each executed run append:

- Environment: Hardware model/QEMU, commit, IDF/build flags, relevant config
- Scenario / command
- Before: `free`, `largest_8bit`
- During/peak (from `upload-measure` logs): min/lowest observed `free`, min/lowest observed `largest_8bit`, phase
- After: `free`, `largest_8bit`
- HTTP status + JSON body
- Filesystem outcome / cleanup observation
- Pass/fail vs expectation and notes

## Baseline observations

**No Hardware or QEMU captures executed in this session.**

- **Executed captures:** none.
- **Blocker (evidence):** ESP-IDF v6.1/QEMU/device capture environment not available in this execution context (no `idf.py` in PATH, no `IDF_PATH`; no attached hardware workflow exercised). Therefore baseline peak + largest-free-block observations cannot be truthfully recorded here.
- **Action required to complete lock:** run capture procedure on Hardware + QEMU (as available/required per table), append dated results/evidence, then finalize threshold/buffer/contract lock sections below.

## Preflight threshold decision (proposed, pending lock)

**Status:** proposed only (no measurement-backed lock).

- **Metric:** `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` checked at upload handler entry (or immediately before parse/buffering path) to protect against fragmentation/contiguous-allocation failures.
- **Proposed initial formula (to validate):** `threshold = parser_alloc_bytes + safety_margin`, where `parser_alloc_bytes = (READ_BUF_SIZE + 1) + ACCUM_BUF_SIZE = 10241` bytes for current buffers, and `safety_margin` covers request/stack/FS write path overhead and fragmentation headroom. **Proposed starting candidate for measurement:** **16 KiB (16384 bytes)** contiguous 8-bit block minimum (rationale: >~10 KB parser buffers plus margin for handler/FS work; must be confirmed/tuned against captured peak/largest-block behavior and failure cases).
- **Rationale:** current multipart parser performs fixed heap allocations per request; largest-block (not only total free) is the limiting factor on fragmented ESP32 heaps. Preflight rejection is intended to fail safely with a clear, contract-compliant error instead of progressing into allocation/parse/write failure paths.
- **Lock criteria:** record observed lowest safe `largest_8bit` for successful supported uploads across scenarios (including fragmentation pressure when feasible), choose final value/formula with margin, and document false-reject/false-accept risks.

## Buffer-size decision and parser-correctness evidence

### Proposed decision (pending lock)

**Proposed:** **keep current `READ_BUF_SIZE` (2048) and `ACCUM_BUF_SIZE` (8192)** for M1 lock unless Hardware/QEMU measurements demonstrate sufficient headroom and parser-correctness cases pass for a smaller configuration.

**Rationale/tradeoffs:**

- **Memory:** current buffers allocate ~10 KB heap per in-flight parse (fixed) plus overhead; reduction would lower peak contiguous demand (goal aligned with parent spec) but must not regress correctness.
- **Correctness risk:** multipart parsing is stateful and boundary/header detection depends on retaining enough unconsumed data in `accum`. The parser keeps a tail of `bm_len` bytes when no boundary is found (`safe = accum_len - bm_len`) and drains on progress; accumulator overflow currently returns `ESP_ERR_NO_MEM`.
- **Throughput/complexity:** smaller `READ_BUF_SIZE` increases recv/loop iterations; smaller `ACCUM_BUF_SIZE` increases risk of overflow/insufficient lookahead for worst-case splits unless logic is proven adequate. Any change requires explicit cases/tests and measurement comparison.
- **M1 position:** no buffer-size change implemented; evidence below identifies/extends runnable host-scope cases for current behavior and decision support. A follow-up change (if justified) belongs in the parent bounds-implementation work after lock or as a separately validated, minimal change with evidence.

### Parser-correctness evidence (runnable / identified)

**Host unit tests (runnable, existing + extended in M1):**

- `test/test_http_multipart.c` (Unity, host): existing static-helper coverage for boundary extraction, part-header parsing, and field-value accumulation/truncation; **M1 extended** with full-stream `http_multipart_parse` cases using mock `httpd_req_recv` body feeding (simple file upload, file + form field, chunked delivery, and representative parse/error paths as feasible in mock scope).
- `test/test_http_upload.c` (Unity, host): upload path/url sanitization helpers (`http_sanitize_upload_path`, `http_url_decode`) — relevant to safe target handling; unchanged functional expectations.

**Additional cases identified for buffer-size/lock validation (add/run when environment/harness supports):**

- Boundary token split across read/accumulator boundaries (including near `bm_len` tail-retention edge).
- Part headers split across reads (`\r\n\r\n` delimiter spanning chunks).
- File payload larger than `ACCUM_BUF_SIZE` streamed across many chunks (no in-RAM file buffering beyond parser buffers).
- Multiple parts with interleaved field and file parts; empty/zero-length part bodies where valid.
- Malformed/truncated bodies: missing closing boundary, premature end (`remaining==0`), invalid/oversized accumulator growth path, and timeout/recv error handling.
- Large `Content-Type`/boundary and long field values (truncation/cap behavior) — ensure no over-read/overflow.
- QEMU/integration multipart endpoint tests for oversize rejection and error contract once enforcement/endpoint harness coverage exists (currently not present as a dedicated HTTP upload integration suite).

**Conclusion (current evidence):** host-scope helper/parse cases can be executed via `make test` when toolchain is available; QEMU/device multipart contract captures are pending (see blockers). Buffer-size reduction is **not justified by evidence in this report** and remains unimplemented.

## Proposed rejection/abort/error contract table (M1 proposal, to lock)

**Status:** proposed concrete schema/status details for approval alongside measurements. Current implementation does **not** fully conform to this target contract; enforcement/alignment is primarily in-scope for the parent bounds-implementation item once locked (except minimal M1 measurement support).

| Case | Trigger (proposed/locked intent) | HTTP status | JSON body (proposed) | Cleanup / write guarantee | Owner/path |
|---|---|---:|---|---|---|
| Preflight low heap | `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` below locked threshold before accepting/buffering as applicable | 503 Service Unavailable | `{"error":"upload_rejected_low_memory","message":"Upload rejected: insufficient contiguous memory"}` | No file opened/written; release request-owned buffers/state | shared upload preflight / core and R24 callers |
| Oversize / unsupported request | Body or upload exceeds locked supported limit (< 64 KB single file) or unsupported multipart form per bounds | 413 Payload Too Large | `{"error":"upload_too_large","message":"Upload exceeds supported limit","max_bytes":65536}` | No partial/corrupt file written beyond defined safe handling; delete/rollback any temp/partial target if created before rejection point | shared parser/validation |
| Parse/malformed request | Malformed multipart/body/headers/encoding within bounded parsing (including invalid boundary/missing terminator/bad structure) | 400 Bad Request | `{"error":"upload_invalid_request","message":"Upload request is malformed or invalid"}` | No partial/corrupt file written; cleanup buffers/temp state | shared parser/validation |
| Stop/abort during in-flight upload | Core HTTP/WS lifecycle stop/abort or equivalent cancellation while upload active (best-effort safe refusal/defer when feasible; otherwise fail in-flight cleanly) | 503 Service Unavailable (or locked request-failure status when response possible) | `{"error":"upload_aborted","message":"Upload aborted"}` (when response can be sent) | **No partial or corrupt file writes** beyond defined safe handling; abort write, close/remove temp/partial artifacts as applicable, release buffers/resources | core stop handler + shared upload cleanup; R24-compatible paths must provide equivalent cleanup and compatible semantics |

**Notes:**

- **Stable machine-readable field:** `error` (stable code). **Human-readable field:** `message`. Optional fields (e.g. `max_bytes`) included where specified. Final field names/values/messages/status choices must be locked with measurement review; dependents must not diverge after lock.
- **Current behavior (observed in code, pre-lock):** handler returns 400 with `{"error":"..."}` for `uctx.error` cases (e.g. missing/invalid path, directory/write/open failures surfaced by upload context) and 500 with messages like `Upload failed: out of memory` / `invalid request` / `incomplete request` / `ESP_ERR_*` for parse failures; no explicit 413/preflight/abort contract yet. M1 records this gap and proposes target contract for lock.
- **Temp/partial file behavior:** current implementation streams directly to target path and closes on final/error in handler path but does not guarantee deletion of partially written targets on all failure modes; locked contract requires explicit no-partial/corrupt-write guarantees and cleanup/rollback semantics (implementation/enforcement tracked in parent).

## Executed captures and blockers

- **QEMU/integration captures:** not executed (no dedicated HTTP upload integration harness/cases in this repo context and no confirmed QEMU/ESP-IDF runtime execution in this session). **Blocker:** environment/harness availability for multipart upload endpoint cases.
- **Hardware/manual captures:** not executed. **Blocker:** no physical device/access and no validated ESP-IDF flash/monitor workflow in this session.
- **Automated checks:** recorded separately in implementation/PR evidence (see plan item progress). This report does not claim unexecuted capture/check results.

## Cross-links and dependent status

- M1 plan item (ready): [`docs/plan/ready/http-upload-measure-and-lock.md`](../plan/ready/http-upload-measure-and-lock.md)
- Parent bounds spec (blocked pending recorded lock): [`docs/plan/needs-clarification/http-upload-memory-bounds.md`](../plan/needs-clarification/http-upload-memory-bounds.md)
- Consolidated tracker: [`docs/plan/consolidated-improvement-plan.md`](../plan/consolidated-improvement-plan.md)
- Dependents remain **blocked** until lock is recorded and approved: `optional-http-websocket-service` upload abort/stop, `r24-berry-script-http-server-api` shared/compatible upload handling. Moving the parent item to `ready` is a follow-up only after recorded/locked results and cross-links are complete.
