# HTTP upload measure-and-lock (M1) report

- **Item:** [`docs/plan/done/http-upload-measure-and-lock.md`](../plan/done/http-upload-measure-and-lock.md)
- **Parent:** [`docs/plan/ready/http-upload-memory-bounds.md`](../plan/ready/http-upload-memory-bounds.md) (M1 lock recorded 2026-11-18; bounds enforcement implementation now unblocked per recorded contract)
- **Date:** 2026-11-18
- **Status:** M1 instrumentation/procedure/parser-correctness cases delivered; **hardware captures recorded (partial, 2026-11-18)** with `upload-measure` evidence and lock decisions recorded below. **QEMU/integration multipart endpoint captures not executed** (no dedicated harness in this session). Some fragmentation-stress/preflight low-heap and core stop/abort-mechanism cases remain not force-reproduced on hardware (see gaps); enforcement/cleanup/behavior changes remain in parent implementation scope.

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

## Baseline observations and executed captures (2026-11-18, hardware, partial)

**Environment (recorded):** physical ESP32 device over serial `COM3` @115200, ESP-IDF v6.1 app `cos` version `f2a8d7d` (compile `Nov 18 2026 15:14:09`, observed in `set-auth.log` boot). Wi-Fi STA connected (`mordor`, IP `192.168.0.187`, AP `192.168.4.1`, signal ~-68 to -74 dBm), core HTTP server running, auth user `cos` (password set to `cos123` during session; update evidenced). SD card mounted; validation uploads targeted `/sdcard/m1tmp/*` (internal flash `/flash` path caused device watchdog/reset boot-loop in an earlier probe — see incident below). Artifacts: `docs/reports/artifacts/hw-2026-11-18-http-upload-m1-com3/` (`console-probe.log`, `sd-prep.log`, `set-auth.log`, `sd-upload-*-serial.log`, `malformed-*-serial.log`, `abort-serial.log`, `m1-probe-serial.log`, `post-crash-probe.log`, `sd-post-check.log`). Generated upload payloads and HTTP response snapshot files were removed as redundant (outcomes/telemetry recorded in serial logs and this report). Serial capture used an ad-hoc PowerShell `System.IO.Ports` fallback (note: control-line toggles caused unintended reboots in some sessions).

**Idle/baseline snapshot (`svc_mem`, `sd-prep.log`):** default/internal free ~29908 B, min ~11908 B, largest block ~12800 B (Berry uninitialized). Values are point-in-time and environment-dependent.

**Executed hardware results (SD path, authenticated `cos:cos123`):**

| Scenario | Request / path (`content_len`) | `upload-measure` observations (key phases) | HTTP result | FS / cleanup observation |
|---|---|---|---|---|
| Supported ~1 KB | `/sdcard/m1tmp/upload-1k.txt` (343) | `handler_start` free=28756 largest_8bit=12800; `parse_alloc_ok` free=18380 largest_8bit=12800; `parse_complete` free=19248 remaining=0 accum_len=4; `handler_success` free=29624 | 200 `{"status":"ok"}` | `Upload complete` logged |
| Supported ~16 KB | `/sdcard/m1tmp/upload-16k.bin` (16729) | `handler_start` free=15260 largest_8bit=12800; `parse_alloc_ok` free=4476 **largest_8bit=2816**; `parse_complete` free=12064 largest_8bit=3328; `handler_success` free=22440 | 200 `{"status":"ok"}` | `Upload complete` logged (serial events captured in `sd-upload-63k-serial.log` due capture overlap/race) |
| Supported ~63 KB (<64 KB) | `/sdcard/m1tmp/upload-63k.bin` (64857) | `handler_start` free=19052 largest_8bit=12800; `parse_alloc_ok` free=11860 largest_8bit=4864; `parse_complete` free=18888 largest_8bit=8192; `handler_success` free=29264 | 200 `{"status":"ok"}` | `Upload complete` logged |
| Oversize >64 KB (current behavior) | `/sdcard/m1tmp/upload-80k.bin` (82265) | `handler_start` free=22460 largest_8bit=12800; `parse_alloc_ok` free=12476 largest_8bit=4864; `parse_complete` free=19268 largest_8bit=5888; `handler_success` free=29644 | 200 `{"status":"ok"}` | Accepted/written per logs (M1 has no enforcement; proposed 413 not implemented) |
| Malformed bad/missing boundary | raw POST, `Content-Type: multipart/form-data` (no boundary) | No `upload-measure` lines captured in that specific serial window (timing/capture contention) | 500 `{"error":"Upload failed: invalid request"}` | HTTP outcome recorded; telemetry capture gap noted |
| Truncated/incomplete multipart (crafted) | `boundary=BOUNDARY`, no closing boundary, `content_len=194` | `Upload complete` `/sdcard/m1tmp/trunc.txt`; `parse_complete` remaining=0 accum_len=7; `handler_success` free=28528 (captured in overlapped log) | 200 `{"status":"ok"}` | Observed parser accepted this crafted body as complete and wrote content (not a parse-failure case) |
| Missing `path` field | file-only multipart, `content_len=217` | `file_callback_rejected` -> `parse_failed` ESP_ERR_INVALID_STATE -> `parse_return` ESP_ERR_INVALID_STATE -> `handler_error_response` **ESP_ERR_INVALID_ARG** | 400 `{"error":"Missing path field"}` | No successful file write logged |
| Abort mid-transfer (client killed) | `/sdcard/m1tmp/abort.bin`, `content_len=82260`, curl killed ~0.6s | `recv_failed` free=18464 largest_8bit=5888 remaining=56582 accum_len=50; `parse_failed` -> `parse_return` ESP_FAIL -> `handler_parse_error`; socket send failed (104) / handler execution failed | No HTTP response delivered | `Upload complete` not logged; `abort.bin` present in listing (`sd-post-check.log`; `ls` readdir branch reports size 0 so size not reliable) — **possible partial file; cleanup not guaranteed** (matches pre-lock gap) |

**Flash-path incident (separate reliability evidence):** authenticated probe to `/flash/m1/probe-1k.txt` (`content_len=338`) emitted `handler_start`/`parse_start_before_alloc` free=18868 largest_8bit=10752 and `parse_alloc_ok` free=8492 largest_8bit=4608, then during LittleFS mkdir/read hit `TG1WDT_SYS_RESET` and entered boot loop with `invalid header: 0xffffffff` (`m1-probe-serial.log`). Device recovered after manual reset; subsequent validation used SD path.

**QEMU/integration:** not executed (no dedicated HTTP upload integration harness/cases and no QEMU runtime exercised in this session). **Gaps/unverified:** forced fragmentation/preflight low-heap stress, core stop/abort-mechanism behavior (only client-abort tested), independent verification of on-disk file sizes/content for SD artifacts, and reproducible capture for bad-boundary telemetry. These gaps are recorded (no false pass claims) but do not invalidate the recorded lock decisions below based on available evidence.

## Preflight threshold decision (locked, 2026-11-18, evidence-based)

**Status:** locked for parent/dependent contract reference (implementation/enforcement remains in parent scope). Based on recorded hardware evidence; subject to revision only with new recorded measurements and coordinated doc updates.

- **Metric:** `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` checked at upload handler entry (or immediately before parse/buffering path) to protect against fragmentation/contiguous-allocation failures.
- **Locked value/formula:** **12288 bytes (12 KiB)** contiguous 8-bit block minimum, i.e. `threshold = parser_alloc_bytes + safety_margin` with `parser_alloc_bytes = (READ_BUF_SIZE + 1) + ACCUM_BUF_SIZE = 10241` and ~2 KB margin for handler/FS/request overhead.
- **Evidence/rationale:** successful SD-path uploads observed pre-alloc/handler-start `largest_8bit` of 12800–28756 bytes (e.g. 16 KB upload: handler/start 12800, alloc succeeded with post-alloc min observed 2816; 63 KB upload: 12800/4864 post-alloc). The earlier 16 KiB candidate would have **false-rejected** feasible uploads in observed states (12800 < 16384 at pre-alloc). 12 KiB admits observed feasible pre-alloc states while reserving > parser allocation; it does **not** guarantee success under severe fragmentation (post-alloc pressure seen) — enforcement must still fail cleanly on allocation/parse errors per locked contract.
- **Risks/limitations:** not force-validated under worst-case fragmentation/near-OOM; QEMU not exercised. Implementers should treat this as the locked initial threshold and record any evidence-driven adjustment as a follow-up contract update (not a silent divergence).

## Buffer-size decision and parser-correctness evidence

### Locked decision (2026-11-18, evidence-based)

**Locked:** **keep current `READ_BUF_SIZE` (2048) and `ACCUM_BUF_SIZE` (8192)**. No buffer-size reduction implemented; reduction is not justified by recorded evidence and would require separate correctness/measurement validation.

**Rationale/tradeoffs (recorded):**

- **Memory:** current buffers allocate ~10 KB heap per in-flight parse (fixed) plus overhead; hardware runs showed successful 1/16/63 KB SD uploads with this configuration (including post-alloc `largest_8bit` pressure in some phases) but no evidence that smaller buffers are safe/correct.
- **Correctness risk:** multipart parsing is stateful and boundary/header detection depends on retaining enough unconsumed data in `accum`. The parser keeps a tail of `bm_len` bytes when no boundary is found (`safe = accum_len - bm_len`) and drains on progress; accumulator overflow currently returns `ESP_ERR_NO_MEM`.
- **Throughput/complexity:** smaller `READ_BUF_SIZE` increases recv/loop iterations; smaller `ACCUM_BUF_SIZE` increases risk of overflow/insufficient lookahead for worst-case splits unless logic is proven adequate. Any change requires explicit cases/tests and measurement comparison.
- **M1 position:** no buffer-size change implemented. A follow-up change (if justified) belongs in parent bounds implementation or a separately validated minimal change with evidence.

### Parser-correctness evidence (runnable / identified)

**Host unit tests (runnable, existing + extended in M1):**

- `test/test_http_multipart.c` (Unity, host): existing static-helper coverage for boundary extraction, part-header parsing, and field-value accumulation/truncation; **M1 extended** with full-stream `http_multipart_parse` cases using mock `httpd_req_recv` body feeding (simple file + form field upload, file-only upload, invalid/missing boundary -> `ESP_ERR_INVALID_ARG`, truncated/incomplete headers -> `ESP_ERR_INVALID_STATE`). Added cases are small-body/single-recv exercises in the current mock scope; multi-recv streaming and boundary/header splits across receive/accumulator boundaries are **not exercised** by these added tests and remain identified cases below.
- `test/test_http_upload.c` (Unity, host): upload path/url sanitization helpers (`http_sanitize_upload_path`, `http_url_decode`) — relevant to safe target handling; unchanged functional expectations.

**Additional cases identified for buffer-size/lock validation (add/run when environment/harness supports):**

- Multi-recv streaming/boundary token split across read/accumulator boundaries (including near `bm_len` tail-retention edge) with bodies/parts forcing more than one `httpd_req_recv` iteration (not exercised by added host cases).
- Part headers split across reads (`\r\n\r\n` delimiter spanning chunks, not exercised by added host cases).
- File payload larger than `READ_BUF_SIZE`/`ACCUM_BUF_SIZE` streamed across many chunks (no in-RAM file buffering beyond parser buffers).
- Multiple parts with interleaved field and file parts; empty/zero-length part bodies where valid.
- Malformed/truncated bodies: missing closing boundary, premature end (`remaining==0`), invalid/oversized accumulator growth path, and timeout/recv error handling.
- Large `Content-Type`/boundary and long field values (truncation/cap behavior) — ensure no over-read/overflow.
- QEMU/integration multipart endpoint tests for oversize rejection and error contract once enforcement/endpoint harness coverage exists (currently not present as a dedicated HTTP upload integration suite).

**Conclusion (current evidence):** host-scope helper/parse cases can be executed via `make test` when toolchain is available; QEMU/device multipart contract captures are pending (see blockers). Buffer-size reduction is **not justified by evidence in this report** and remains unimplemented.

## Rejection/abort/error contract table (locked, 2026-11-18)

**Status:** locked as the shared contract for parent implementation and dependents (core stop/abort, R24 shared/compatible paths). Recorded from M1 proposal with evidence-informed review; **current implementation does not yet fully conform** (enforcement/alignment/cleanup guarantees are in-scope for parent/dependent implementation work). Dependents must not diverge after lock.

| Case | Trigger (proposed/locked intent) | HTTP status | JSON body (proposed) | Cleanup / write guarantee | Owner/path |
|---|---|---:|---|---|---|
| Preflight low heap | `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` below locked threshold before accepting/buffering as applicable | 503 Service Unavailable | `{"error":"upload_rejected_low_memory","message":"Upload rejected: insufficient contiguous memory"}` | No file opened/written; release request-owned buffers/state | shared upload preflight / core and R24 callers |
| Oversize / unsupported request | Body or upload exceeds locked supported limit (< 64 KB single file) or unsupported multipart form per bounds | 413 Payload Too Large | `{"error":"upload_too_large","message":"Upload exceeds supported limit","max_bytes":65536}` | No partial/corrupt file written beyond defined safe handling; delete/rollback any temp/partial target if created before rejection point | shared parser/validation |
| Parse/malformed request | Malformed multipart/body/headers/encoding within bounded parsing (including invalid boundary/missing terminator/bad structure) | 400 Bad Request | `{"error":"upload_invalid_request","message":"Upload request is malformed or invalid"}` | No partial/corrupt file written; cleanup buffers/temp state | shared parser/validation |
| Stop/abort during in-flight upload | Core HTTP/WS lifecycle stop/abort or equivalent cancellation while upload active (best-effort safe refusal/defer when feasible; otherwise fail in-flight cleanly) | 503 Service Unavailable (or locked request-failure status when response possible) | `{"error":"upload_aborted","message":"Upload aborted"}` (when response can be sent) | **No partial or corrupt file writes** beyond defined safe handling; abort write, close/remove temp/partial artifacts as applicable, release buffers/resources | core stop handler + shared upload cleanup; R24-compatible paths must provide equivalent cleanup and compatible semantics |

**Notes (locked):**

- **Stable machine-readable field:** `error` (stable code). **Human-readable field:** `message`. Optional fields (e.g. `max_bytes`) included where specified. Locked for dependent use; changes require recorded contract update.
- **Current behavior (observed pre-enforcement):** handler returns 400 with `{"error":"..."}` for `uctx.error` cases and 500 for parse failures (`Upload failed: ...` / `ESP_ERR_*`); no explicit 413/preflight/abort contract yet. Hardware evidence also showed oversize 80 KB accepted (200) and client-abort producing recv/parse failure with possible partial file risk on direct-write path.
- **Temp/partial file behavior:** current implementation streams directly to target path and does not guarantee deletion of partially written targets on all failure/abort modes (abort probe observed). Locked contract requires explicit no-partial/corrupt-write guarantees and cleanup/rollback semantics (implementation/enforcement tracked in parent/dependents).
- **Hardware evidence informing lock:** 1/16/63 KB SD uploads succeeded; 80 KB accepted (200) under current behavior; missing-path rejected (400, `Missing path field`); client abort mid-transfer caused recv/parse failure (ESP_FAIL) and no delivered response with possible partial artifact. Bad-boundary request returned 500 `Upload failed: invalid request` (telemetry capture gap in that run). These observations align with locking the proposed contract for enforcement work.

## Executed captures and blockers

- **QEMU/integration captures:** not executed. **Blocker/gap:** no dedicated HTTP upload integration harness/cases and no QEMU multipart endpoint capture run in this session.
- **Hardware/manual captures:** **executed (partial, 2026-11-18, COM3/STA 192.168.0.187, SD path)** — supported 1/16/63 KB uploads, 80 KB oversize (current behavior), malformed cases (bad boundary, crafted truncated body, missing path), and client-abort mid-transfer recorded with `upload-measure`/HTTP/FS observations as above. **Gaps:** forced fragmentation/preflight low-heap stress and core stop/abort-mechanism not reproduced; some telemetry/file-size verification limitations noted; flash `/flash` path probe caused device boot-loop incident (separate reliability concern).
- **Automated checks (earlier M1 implementation session, recorded):** `make format-check` **pass**; `make test` **pass** (CTest 30/30 incl. `test_http_multipart`/`test_http_upload`, Python scripts tests 3/3 OK). `make build`/`make lint` not run (ESP-IDF env unavailable), `docs-check`/integration-test runs not executed as documented in plan progress. No new firmware code changes were made during this hardware-capture doc update.

## Cross-links and dependent status

- M1 plan item (done): [`docs/plan/done/http-upload-measure-and-lock.md`](../plan/done/http-upload-measure-and-lock.md)
- Parent bounds spec (unblocked for implementation by recorded lock): [`docs/plan/ready/http-upload-memory-bounds.md`](../plan/ready/http-upload-memory-bounds.md)
- Consolidated tracker: [`docs/plan/consolidated-improvement-plan.md`](../plan/consolidated-improvement-plan.md)
- **Prerequisite satisfied by this recorded lock:** `optional-http-websocket-service` upload abort/stop and `r24-berry-script-http-server-api` shared/compatible upload handling may proceed against the locked threshold/buffer/contract (implementation must still satisfy locked cleanup/no-partial-write semantics and not diverge). QEMU/remaining hardware stress gaps should be closed during implementation validation where feasible.
