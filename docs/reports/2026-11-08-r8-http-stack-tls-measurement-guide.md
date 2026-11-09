# R8 — HTTP Worker Stack/TLS Measurement & Safe Tuning Guide

**Date:** 2026-11-08 (updated with software safety fix)  
**Plan:** `docs/2026-11-08-consolidated-improvement-plan.md` R8  
**Source:** `docs/2026-11-06-memory-pressure-fixes.md` P4/P5  
**Hardware:** CYD / ESP32-2432S028R (or equivalent supported board) with Wi-Fi; SD card optional for Berry test scripts.

## Scope

R8 validates and safely tunes HTTP worker memory behavior:

- **P4:** Measure HTTP worker stack high-water mark (HWM) under representative cases; reduce `HTTP_TASK_STACK` only with measured peak + >=25% margin.
- **P5:** Profile HTTPS/TLS peak heap, largest free block, and cleanup; **do not disable certificate verification**.
- **R2 follow-up (software done in R8):** Timeout/abandon response-buffer safety — on caller timeout, fetch returns a stable empty response snapshot and worker/event paths stop mutating public response buffers for that request.

## Software state (2026-11-08)

Implemented in firmware (hardware validation still required):

- `components/network/src/network_http.c` logs `[R8/P4 stack]` after each worker request: configured stack, `StackType_t` unit, HWM words/bytes, used/peak estimate, method/scheme/status/error.
- `network_http_get_worker_stack_info()` exposes the last sample.
- `berry profile` prints HTTP worker stack info when available.
- `HTTP_TASK_STACK` remains **6144 bytes** (prior reduction from 12288; R8 must revalidate before any further reduction).
- Timeout safety: on timeout, `network_http_fetch` sets abandoned state, clears the public `network_http_response_t` to a safe empty snapshot (`status=0`, no headers/body, `body` points to stable empty storage), and returns `ESP_ERR_TIMEOUT`. `http_event`/`perform_request` check abandonment before mutating shared response buffers. Worker remains busy until `esp_http_client_perform` completes; next fetch returns `ESP_ERR_INVALID_STATE` until then.

## Preconditions

1. Flash firmware containing R8 changes.
2. Board has working serial console and, for HTTP tests, Wi-Fi credentials configured/connected.
3. Use `logging info` if stack/HWM logs are not visible at default level (R8 stack log uses `ESP_LOGI`).
4. Prefer clean boot before HTTPS/peak measurements.
5. Record `idf.py size` / `idf.py size-components` for the build under test when comparing binaries.

## Commands and data sources

| Purpose | Command / source |
|---|---|
| Heap/Berry/worker snapshot | `berry profile` |
| Focused R8 HTTP worker sample | `berry http-stack` (alias: `berry net-stack`) |
| Current/min/largest heap | `mem` |
| Force Berry GC | `berry gc` |
| HTTP worker stack log | Serial log line `R8/P4 stack: ...` after each fetch |
| Wi-Fi | `wifi status`, `wifi connect ...` as applicable |

Capture the full `R8/P4 stack` log line for every test case. `berry http-stack` is convenient for manual matrix recording after each fetch; serial logs remain authoritative when a run produced multiple samples or when diagnosing failures.

## P4 — Stack HWM matrix

Run each case at least once; repeat HTTPS/max/timeout/error cases 3 times and record worst (lowest HWM bytes / highest used estimate).

| Case | How to trigger | Record |
|---|---|---|
| HTTP GET (small) | `net.fetch("http://<reachable>/small")` via Berry script or existing test harness | method, scheme, status, err, cfg, unit, hwm_words, hwm_bytes, used~ |
| HTTP POST with body | fetch with `method=POST`, small JSON/text body | same |
| HTTPS GET | `net.fetch("https://<valid CA>/", ...)` after clean boot, cert verification enabled | same + heap before/after (`mem`/`berry profile`) |
| HTTPS POST | POST over HTTPS to a controlled endpoint if available; otherwise document N/A | same |
| Max response | endpoint returning body near `NETWORK_HTTP_BODY_MAX` (2048) | status/err must reflect truncation/`ESP_ERR_INVALID_SIZE` behavior if oversized; record stack and heap |
| Timeout | short `total_timeout_ms` / unreachable endpoint causing caller timeout | stack log after worker eventually completes; note that caller gets timeout and response is empty snapshot |
| Transport/error | DNS failure, connect failure, TLS failure path as available | stack/error |

### Stack tuning decision

- **Do not reduce** `HTTP_TASK_STACK` unless worst-case measured peak stack (configured - HWM bytes, interpreted with correct `StackType_t` unit) leaves **>=25% margin** below configured stack.
- If margin is insufficient, **increase** stack or defer reduction with evidence.
- Account for ESP-IDF port stack units (`sizeof(StackType_t)`); the log prints `unit`.
- Re-run HTTPS and timeout/error cases after any stack change.

## P5 — HTTPS/TLS peak memory

For each HTTPS case (prefer clean boot, note BT/Wi-Fi state):

1. Record `mem` and `berry profile` before request.
2. Run HTTPS fetch.
3. Record `mem` and `berry profile` after request and after `berry gc` if useful.
4. Check `heap_min`/largest block regression and any `ESP_ERR_NO_MEM`/PSA/TLS errors.

**Pass criteria (P5):**

- Certificate verification remains enabled (no insecure flags).
- No use-after-free/crash on HTTPS success or failure.
- Peak/largest-block behavior documented; if PSA/X.509 reports insufficient memory (e.g. `-141`), record exact endpoint, boot state, BT state, and keep as R8/R2 follow-up evidence.

## Timeout/abandon safety (R8 software fix — validate)

| Check | Expected |
|---|---|
| Request times out (`total_timeout_ms` exceeded) | `net.fetch` returns error/timeout result; Berry binding reports error; no crash |
| Immediate follow-up fetch while worker still busy | Returns `ESP_ERR_INVALID_STATE` (or binding surfaces equivalent) until worker completes |
| Follow-up fetch after worker completes | Succeeds normally if network/path valid |
| Repeated timeout + later success | No progressive heap corruption, no stale body/header data observed |

**Do not** rely on response body/header contents after timeout; R8 makes the native response a safe empty snapshot, but callers should treat timeout as an error.

## All-in-one validation script (automatable subset)

`storage/flash_data/tests/r8/http_validation.be` runs an automatable R8 subset and prints snapshots (`mem`, `berry http-stack`, `berry profile`) around each case. Deploy it to the device (`/flash` or `/sdcard`) and capture full serial output, including `R8/P4 stack:` lines.

```text
berry exec /flash/tests/r8/http_validation.be [http_get_url] [https_url] [post_url] [max_body_url]
```

Defaults: `http_get_url=http://example.com/`, `https_url=https://example.com/`. Pass `-` to skip optional positional args. `post_url`/`max_body_url` are skipped unless provided (use controlled endpoints).

The script covers: baseline snapshot, HTTP GET, optional POST+body, HTTPS GET + GC/snapshot, optional max/large response, timeout/abandon with immediate follow-up, and 3 repeated HTTP GETs + GC/snapshot. It **does not** replace: clean-boot/BT-state control for P5, `idf.py size` comparisons, authoritative per-request serial HWM logging, controlled max-body server setup, or worst-case 3× repeats for stack-margin decisions.

## Suggested Berry snippets (adapt to environment)

```berry
import net

# HTTP GET
var r = net.fetch("http://example.com/", {"timeout_ms": 10000, "total_timeout_ms": 10000})
print(r["ok"], r["status"], r["error"])

# Timeout safety: use an unreachable/slow endpoint or very small timeout as appropriate
var t = net.fetch("http://10.255.255.1/", {"timeout_ms": 100, "total_timeout_ms": 100})
print(t["ok"], t["status"], t["error"])
# immediate retry may be ESP_ERR_INVALID_STATE while worker busy
var t2 = net.fetch("http://example.com/")
print(t2["ok"], t2["status"], t2["error"])
```

## Evidence to record in plan/report

- Firmware commit/build identifier and `idf.py size` if compared.
- Full P4 matrix rows (including HWM values).
- P5 heap snapshots and any TLS errors.
- Timeout/abandon validation results.
- Decision: keep `HTTP_TASK_STACK=6144`, increase, or reduce (with measured margin).

## Status

- Software instrumentation + timeout/abandon safety: **implemented in repo**.
- Hardware measurements/tuning decisions: **pending user validation** (partial evidence recorded below).

## Results (2026-11-09, hardware evidence — Wi‑Fi connected, `http_validation.be`)

> Sources: user-provided serial outputs after Wi‑Fi connection (multiple runs, including oversized and 3× repeat attempts). Not clean-boot P5 baselines; BT state not recorded; no `idf.py size` delta for R8. Public endpoints used (`example.com`, `postman-echo.com`, `httpbingo.org`) — service/TLS behavior can vary.

### Summary

| Case | Representative result | HWM / stack | Notes |
|---|---|---:|---|
| HTTP GET `http://example.com/` | 200 OK, `body_len=577` | best 4488/6144; later global sample 2184/6144 | Stable across runs. |
| HTTPS POST `https://postman-echo.com/post` (JSON body) | **Success in early run:** 200 OK, `body_len=322`. **Later runs:** `ESP_ERR_HTTP_CONNECT` with PSA/X.509 cert verify `-141` (`0xffffff73`) | **`2184/6144`, `used~3960` (~35.5% margin)** | Worst stack sample observed. Failures correlate with extreme heap pressure/watermarks in long session. |
| HTTPS GET `https://example.com/` | 200 OK in several runs; later `ESP_ERR_HTTP_CONNECT` with `-141` in degraded heap state | `2184/6144` (observed on error path too) | Cert verification enabled. |
| Max body `https://httpbingo.org/bytes/2000` (2000 bytes) | 200 OK, `body_len=2000` | `2184/6144` | Within `NETWORK_HTTP_BODY_MAX=2048`. |
| Oversized `https://httpbingo.org/bytes/2100` | **`ok=false status=0 body_len=0 error=ESP_ERR_INVALID_SIZE`** | `2184/6144`, `status=0 err=ESP_ERR_INVALID_SIZE` | Correct bounded-body failure; no crash. |
| Timeout target `http://10.255.255.1/...` (`100/200ms`) | `ESP_ERR_HTTP_CONNECT` (esp-tls/transport select/connect failure) | n/a (completed as transport error) | **Not** a true caller timeout/worker-abandon case. Immediate follow-up succeeded when prior case completed normally. |
| Repeated HTTP GET ×3 (per run) | all 200 OK | stable | No progressive heap decrease after GC in each run; `largest` remained 13824–22528 depending on session state. |

### Clean-boot P5 run (2026-11-09)

Pre-test: `mem`/`berry profile` showed healthy state (`min 68872`, largest 65536, Berry not initialized, HTTP worker not started). Wi‑Fi: `AP+STA`, STA `mordor` 192.168.0.187, signal `-69 dBm` (BT state not explicitly recorded).

Command: `http_validation.be https://example.com/ https://example.com/ https://postman-echo.com/post https://httpbingo.org/bytes/2000`

| Case | Result | HWM/heap | Notes |
|---|---|---|---|
| HTTPS GET `example.com` | 200 OK, 577 bytes | `2272/6144`, `used~3872`; min dropped to 6776 | OK. |
| HTTPS POST `postman-echo.com` | **`ESP_ERR_HTTP_CONNECT`**, PSA cert verify **`-141`** | `2272/6144`; min dropped to 1968 | Clean-boot-adjacent failure after first HTTPS GET. |
| HTTPS GET `example.com` | 200 OK | `2272/6144`; min 692 | Recovered/succeeded despite low watermark. |
| Max HTTPS `httpbingo.org/bytes/2000` | 200 OK, 2000 bytes | `2272/6144` | OK. |
| Timeout `10.255.255.1` | `ESP_ERR_HTTP_CONNECT` | transport completion | Immediate HTTPS follow-up succeeded (200). |
| Repeated HTTPS GET ×3 (`example.com`, script used `http_get` arg) | all 200 OK | stable after GC | No progressive heap decrease. |

### True caller-abandon evidence (2026-11-09)

Manual snippet: `net.fetch("http://httpbingo.org/delay/5", {"timeout_ms":300,"total_timeout_ms":300})` then two follow-ups to `example.com`.

Observed:
- `delay: false 0 ESP_ERR_TIMEOUT` — caller deadline path returned timeout.
- `followup: false 0 ESP_ERR_INVALID_STATE` — immediate follow-up rejected while worker still busy (abandoned request still running in background).
- `retry: false 0 ESP_ERR_INVALID_STATE` — retry executed immediately after follow-up (no wait) while the ~5s delayed request was still occupying the worker.

**Interpretation:** validates R8 behavior for the caller-abandon case: timeout returns promptly with safe error/empty response semantics; while the abandoned worker is still active, new fetches are correctly rejected as `ESP_ERR_INVALID_STATE`; no crash/stale-data exposure observed. A later retry *after worker completion* was not captured in this snippet (retry was immediate), but the busy serialization is confirmed.

### Key observations

- **P4 stack (decision-relevant):** `HTTP_TASK_STACK=6144`, `unit=1` byte. Worst observed HWM **2184 bytes remaining** (`used~3960`) on HTTPS POST and on HTTPS/oversized error paths; clean-boot run observed **2272** on HTTPS. Margin **~35.5–37%** (>25%). No stack HWM below 2184 observed across all captured runs. **Decision:** keep `HTTP_TASK_STACK=6144`; do not reduce without new worst-case evidence.
- **P5 heap/TLS (critical follow-up, R2):** HTTPS succeeded in healthy states and `httpbingo.org/bytes/2000` remained reliable in captured runs, but PSA cert verification failed with **`-141` insufficient memory** for `postman-echo.com` and sometimes `example.com` under heap pressure (clean-boot-adjacent and long-session). Heap watermarks became extreme (down to **84** bytes in long-session snapshots; **692** in clean-boot run). No crash in captured output; GC recovered free heap. **HTTPS reliability under constrained heap remains fragile** and needs targeted R2 investigation (BT coexistence, cert bundle/TLS/PSA memory options with verification enabled, sequencing/policy).
- **Response limits:** 2000-byte response OK; 2100-byte response correctly fails as `ESP_ERR_INVALID_SIZE`.
- **Timeout/abandon:** Transport-failure path stable. **True caller-abandon path now has hardware evidence** (delay endpoint): `ESP_ERR_TIMEOUT`, then `ESP_ERR_INVALID_STATE` while abandoned worker busy; no crash/stale data. Later-post-completion retry was not captured (immediate retry in snippet) but can be confirmed by waiting >5s and retrying.

### Remaining actions (post-R8 software completion)

1. **Clean-boot P5:** reboot, record BT/Wi‑Fi state, run HTTPS GET/POST; capture min heap/largest block and any `-141`. Prioritize R2 HTTPS-memory follow-up.
2. **True caller-abandon repro (best effort):** if needed, test slow endpoints with equal short timeouts (e.g. `/delay/*` with `timeout_ms=300,total_timeout_ms=300`) and record whether fetch returns `ESP_ERR_TIMEOUT`/busy `ESP_ERR_INVALID_STATE` behavior. Document if not reproducible given API/client constraints.
3. **Build metadata:** record firmware build/commit and `idf.py size`/`size-components` for the tested image.
4. **Product/heap follow-up (separate from R8 tuning):** investigate PSA `-141` under low heap/BT coexistence; consider memory/connection policies, cert bundle/TLS config options compatible with verification, or sequencing to avoid HTTPS bursts in degraded heap states.

## True caller-abandon test snippet (best effort, for item 3)

The fetch API validates `total_timeout_ms >= timeout_ms`, so caller-abandon-while-client-busy is difficult to trigger externally. Try (endpoint availability varies):

```berry
import net
# Attempt: short equal timeouts against a delayed endpoint
var t = net.fetch("http://httpbingo.org/delay/5", {"timeout_ms": 300, "total_timeout_ms": 300, "method": "GET"})
print("delay:", t["ok"], t["status"], t["error"])
var t2 = net.fetch("http://example.com/", {"timeout_ms": 5000, "total_timeout_ms": 8000, "method": "GET"})
print("followup:", t2["ok"], t2["status"], t2["error"])
# If worker was abandoned and still busy: followup error may be ESP_ERR_INVALID_STATE; retry later.
var t3 = net.fetch("http://example.com/", {"timeout_ms": 5000, "total_timeout_ms": 8000, "method": "GET"})
print("retry:", t3["ok"], t3["status"], t3["error"])
```

Interpretation: `ESP_ERR_HTTP_CONNECT`/other transport errors mean normal completion (not abandon). `ESP_ERR_TIMEOUT` from fetch may indicate caller deadline path; check immediate follow-up for `ESP_ERR_INVALID_STATE`. If not reproducible, record the attempts in the R8/R2 evidence.
