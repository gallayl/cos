# R8 Follow-ups — HTTPS/PSA `-141` Heap Pressure (R2 P5)

**Date:** 2026-11-09  
**Source:** R8 hardware validation (`docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`) and R2 follow-ups in `docs/plan/consolidated-improvement-plan.md`.  
**Priority:** High (correctness/reliability for HTTPS).  
**Constraint:** **Do not disable certificate verification.**

## Problem

During R8 hardware validation (Wi‑Fi connected, CYD/ESP32-2432S028R class target, ESP-IDF 6.1), HTTPS requests sometimes failed with PSA/X.509 certificate verification insufficient memory:

```text
esp-x509-crt-bundle: PSA signature verification failed with error 0xffffff73 (decimal: -141)
esp-x509-crt-bundle: Certificate matched but signature verification failed
esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x3000
HTTP_CLIENT: Connection failed, sock < 0
```

`net.fetch` reported `ok=false status=0 error=ESP_ERR_HTTP_CONNECT`. No crash or stale response corruption was observed in captured runs; Berry GC recovered free heap after failures.

## Evidence (2026-11-09)

| Scenario | Result | Heap notes |
|---|---|---|
| Early healthy run | HTTPS GET `example.com` 200 OK; HTTPS POST `postman-echo.com` 200 OK; HTTPS GET `httpbingo.org/bytes/2000` 200 OK (2000 bytes) | HTTPS POST peak watermark `min=1272` (recovered). |
| Clean-boot-adjacent run (AP+STA Wi‑Fi, `mordor` -69 dBm; BT state not recorded) | HTTPS GET `example.com` 200 OK; HTTPS POST `postman-echo.com` **`-141`**; subsequent HTTPS GET `example.com` 200 OK; `bytes/2000` 200 OK; repeated HTTPS GET ×3 200 OK | Min fell to 1968/692 around HTTPS failures/success transitions. |
| Long-session repeated runs | `postman-echo.com` POST and later `example.com` GET failed with **`-141`**; `httpbingo.org/bytes/2000` still succeeded in the same degraded runs; HTTP GET/repeated HTTP success still OK | Heap minima degraded to 484/264/228/**84** bytes in snapshots. |

R8 stack decision is independent: `HTTP_TASK_STACK=6144` kept (worst HWM 2184, ~35.5% margin). Stack is not the primary failure mode.

## Working hypotheses

1. **PSA/mbedTLS/X.509 verification peak RAM** exceeds available contiguous/internal heap under certain runtime states (fragmentation, coexisting allocations, Wi‑Fi/Berry/other services).
2. **Endpoint/cert-chain differences** affect peak usage (`postman-echo.com` failed more readily than `httpbingo.org/bytes/2000` in degraded runs).
3. **BT coexistence** may reduce available RAM/fragmentation headroom (not measured in these runs).
4. **Long-session fragmentation/watermark exhaustion** makes later HTTPS more fragile even after GC frees total bytes.

## Investigation checklist (safe)

1. **Reproduce from clean boot** with explicit state capture:
   - `mem`, `berry profile`, `wifi status`, `bt`/Bluetooth state (if applicable).
   - Single HTTPS GET to a known-good endpoint, then single HTTPS POST; record min/largest/internal minima and any `-141`.
2. **Record BT impact**: compare HTTPS behavior with BT/HID disabled or absent vs enabled/connected (if product supports both).
3. **Compare endpoints/cert chains**: `example.com`, `postman-echo.com`, `httpbingo.org`, and any production endpoints; note which chains trigger `-141`.
4. **Inspect ESP-IDF 6.1 / mbedTLS 4.x config** (no insecure changes): buffer sizes, PSA crypto/driver memory, cert bundle verification path, client-only options, dynamic buffer settings. Use `idf.py menuconfig`/`sdkconfig` evidence.
5. **Measure peak allocations** if tooling is available: heap trace/caps tracing around handshake; focus on internal/8-bit capable allocations if PSA/cert code requires them.
6. **Evaluate mitigations** (only with verification preserved): sequencing (avoid HTTPS bursts), freeing/releasing nonessential services before critical HTTPS, reducing unrelated peak allocations, targeted config changes supported by ESP-IDF 6.1, or product policy (e.g. defer HTTPS when heap below threshold — report clearly).

## Non-goals / guardrails

- Do **not** disable certificate verification.
- Do **not** “fix” by merely hiding errors; report failures clearly.
- Do **not** reduce `HTTP_TASK_STACK` below 6144 without new >=25% worst-case HWM evidence.
- Treat changes as measured experiments; capture before/after heap, HWM, and success/failure matrices.

## Acceptance criteria for the follow-up

- Reproducible measurement matrix for `-141` (endpoint, boot state, BT state, heap minima/largest).
- A documented mitigation or explicit product limitation with user-facing behavior.
- HTTPS success on agreed target endpoints from clean boot (or clear, actionable failure reporting) while keeping certificate verification enabled.
- No regressions to R8 results: stack margin >=25%, oversized body fails as `ESP_ERR_INVALID_SIZE`, timeout/abandon safety intact, no progressive heap leak after repeated successful HTTP operations.

## Related completed R8 items

- Timeout/abandon safety software fix and hardware abandon-path evidence (`ESP_ERR_TIMEOUT` + busy `ESP_ERR_INVALID_STATE`) are complete; optional post-completion retry capture can be done by waiting >5s after the delay test and issuing a successful fetch.
- `berry http-stack` and `storage/flash_data/tests/r8/http_validation.be` remain useful for regression checks.
