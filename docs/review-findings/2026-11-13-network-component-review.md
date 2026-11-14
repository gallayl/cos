# Network component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/network/` (`include/network.h`, `src/network.c`, `src/network_url.c`, `src/network_http.c`, `src/network_status.c`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, resource handling, concurrency assumptions, and maintainability of the network module as-is.

## What this repo does

COS is ESP32 (CYD/ESP32-2432S028R) firmware with a display/touch UI and serial console. `components/network` provides small network helpers used by the console, Berry runtime, and status APIs: URL/base-URL normalization and composition, Wi-Fi status/connect wrappers, IPv4 DNS resolve and ICMP ping on a single worker task, and HTTP(S) fetch on a separate single-flight worker task (shared request/response buffers, bounded timeouts, timeout/abandon handling, stack-HWM sampling).

**Load assumed:** one user at a time, interactive console/Berry script use, occasional HTTP fetches and ping/resolve requests (not many concurrent network API callers). Ping/resolve and HTTP are each designed for one request at a time; callers are rejected while a previous timed-out worker operation is still finishing.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| N1 | Should fix | Base URL state (`s_base`) is unsynchronized across set/get/compose/fetch | Open |  | Concurrent set + compose/fetch can race on shared RAM; current call pattern is mostly single-threaded |
| N2 | Should fix | HTTP worker lazy init is not synchronized | Open |  | First concurrent `network_http_fetch` calls can race `ensure_http_worker()` and leak/overwrite queue, semaphores, task handle |
| N3 | Nice to have | Hostname validation logic is duplicated | Open |  | `valid_hostname` implemented separately in `network.c` and `network_url.c` |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### Must fix

No must-fix defects found in the reviewed code for the assumed load. Timeout/abandon behavior in ping and HTTP is intentionally documented (caller gets a bounded result/snapshot; a stale worker operation can keep the single-flight service busy until it finishes) and matches the current API contract.

### Should fix

1. **Unsynchronized base URL (`network_url.c:9`, `network_url.c:651-733`, `network_http.c:532`)**
   - **What this is:** `network_set_base_url`, `network_get_base_url`, and `network_compose_url` use module-global `s_base` with no lock. `network_http_fetch` composes the request URL while holding `s_http_mutex`, but `network_compose_url` reads `s_base` directly.
   - **Problem:** A concurrent writer and reader can race on `s_base` bytes. `network_compose_url` may parse/copy a partially updated base URL, and `network_set_base_url`/`network_get_base_url` can read or write while another thread is reading.
   - **Fix:** Guard `s_base` with a small mutex (or reuse an existing network mutex dedicated to URL state), copy the base to a local buffer under the lock, then run validation/formatting on the local copy. Keep the change internal to `network_url.c`/call sites.
   - **If we skip it:** Under the current mostly single-threaded script/console use this is unlikely, but a future or accidental concurrent `net.set_base_url` and HTTP fetch/compose call can produce a wrong URL, rejected request, or memory corruption-like parse behavior.

2. **HTTP worker lazy initialization race (`network_http.c:432-473`)**
   - **What this is:** `ensure_http_worker()` checks global queue/mutex/semaphore/task pointers without synchronization, creates them, assigns globals, then creates `net_http` with `xTaskCreate`.
   - **Problem:** Two tasks entering the first `network_http_fetch` at the same time can both observe missing worker state and both create resources. One task's handles can overwrite the other's globals, leaking a queue/semaphores/task handle and leaving callers using different objects than the running worker expects.
   - **Fix:** Serialize worker creation (for example with a static init mutex/critical section, or create the worker from `network_init` and make `network_http_fetch` require it like ping/resolve do). Keep `network_http_get_worker_stack_info` valid before first request if callers rely on configured-stack defaults.
   - **If we skip it:** Safe for the assumed single-caller load, but concurrent first-use fetches can leak RTOS objects and put the HTTP service into an inconsistent state.

### Nice to have

3. **Duplicate hostname validation (`network.c:66-105`, `network_url.c:117-156`)**
   - **What this is:** `network.c` and `network_url.c` each implement nearly the same `valid_hostname` rules (labels, dots, hyphens, max lengths), with small differences in null/length checks (`strnlen(..., NETWORK_HOST_MAX + 1)` vs `strlen`).
   - **Problem:** A future rule change (IDN handling is not present, but normal hostname limits could evolve) must be updated in two places and can drift.
   - **Fix:** Move hostname validation to one internal helper shared by the network sources, or make one implementation call the other; keep behavior identical for current valid/invalid inputs.
   - **If we skip it:** No current functional bug observed; only maintenance risk.

## Verdict

The module is careful about the hard parts it already addresses: bounded buffers, single-flight ping and HTTP workers, explicit timeout/abandon contracts, and stable timeout response snapshots for HTTP. I did not find a must-fix correctness or security defect in the reviewed code for the assumed single-user, occasional-use load. The main risks are concurrency assumptions that are not enforced for base-URL state and first-use HTTP worker creation; they matter if call patterns become concurrent.

## Lean

`Lean: -0 lines, -0 dependencies possible.` (One small duplication can be removed by sharing `valid_hostname`, but that is a refactor, not clearly fewer total concepts; no obvious unused dependency in `CMakeLists.txt` from this review.)

## Not checked

- On-device ESP-IDF build and hardware behavior for Wi-Fi connect/disconnect, DNS, ICMP ping, HTTP, and HTTPS/TLS (`esp_http_client` + cert bundle). ESP-IDF toolchain/hardware was not exercised in this review.
- Concurrent callers across tasks (Berry script, console command, status readers) beyond static reasoning; review assumed single-user, occasional use.
- Real timeout/abandon races on hardware (cases where DNS/ICMP/HTTP continues after caller deadline and a later request arrives immediately), and stack high-water-mark margins under maximum TLS/response workloads (existing R8/P4 guidance/reports were not re-measured).
- `network_init` failure after partial object creation is not reachable in the current simple init path except allocation/task-creation failures, but callers outside `main` are not audited.
- `test/test_network_url.c` and host/network mocks after any fix (run relevant checks per `AGENTS.md` when implementing).
- Behavior of `network_get_wifi_info`/`network_get_host_info` when Wi-Fi/netif is partially initialized or in AP/STA transitions beyond the direct API calls observed.
