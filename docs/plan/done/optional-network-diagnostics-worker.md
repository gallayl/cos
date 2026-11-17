# Optional network diagnostics worker lifecycle

- **ID:** optional-network-diagnostics-worker
- **Status:** done (implementation delivered; hardware/manual validation pending human sign-off)
- **Created:** 2026-11-16
- **Updated:** 2026-11-17
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:135`, `components/network/src/network.c:18-24`, `components/network/src/network.c:281-325`. PR: https://github.com/gallayl/cos/pull/52
- **Approval:** approved (2026-11-16)

## Summary
Network diagnostics (`ping`/`resolve` worker) are initialized at boot and always create synchronization objects plus a dedicated task/stack. This reserves RAM even when diagnostics are never used. The feature should make this worker optional or on-demand with clear cleanup behavior.

## Scope
- Make diagnostics worker resources created lazily on first use and/or controllable via service lifecycle.
- Define cleanup/stop behavior for queue, semaphores, task, and in-flight/abandoned requests.
- Preserve existing `network_ping`/`network_resolve` semantics, timeout behavior, and single-flight constraints unless explicitly changed.

## Unified lifecycle / observability mapping (resolved, 2026-11-16 review)
- **Descriptor/boot representation:** represent diagnostics as a boot-time unified-lifecycle service descriptor (e.g. `network_diagnostics`) with a cheap idempotent init/ensure hook at boot that allocates **no persistent** worker task/queue/mutex/ping-completed semaphore. `network_init()` remains a compatible wrapper/ensure hook in profiles/boots where the diagnostics service is available.
- **Boot-profile precedence:** when the active boot profile forces diagnostics unavailable (`low-memory-offline` per `boot-profiles-feature-modes.md`), do **not** run diagnostics boot ensure/init and do **not** lazy-create worker resources on first use. `network_ping`/`network_resolve` (and user-facing `ping`/`resolve`) must report a clear disabled-by-profile/unavailable error/state (no auto-ensure side effect). Descriptor/state must be reported as `disabled-by-profile`/`unsupported-in-profile` (canonical shared vocabulary term) rather than `idle`/`ready`.
- **Lifecycle states (when available/allowed):** **`uninitialized`/`idle`** (observable at boot, no persistent diagnostics worker resources allocated) before first use. **`running`/`ready`** after a **successful** lazy-create/ensure on `network_ping`/`network_resolve` (resources kept alive until reboot per Q1/Q2 decisions). If lazy-create/ensure fails (e.g. allocation/task creation failure, `ESP_ERR_NO_MEM`), report canonical **`failed`** state (with clear error context where feasible), keep **no persistent** worker resources allocated, and **allow later `network_ping`/`network_resolve` calls to retry** lazy creation (idempotent/thread-safe). Report init/availability/errors clearly using canonical shared state strings from the shared diagnostics contract.
- **Capabilities:** runtime **stop/deinit unsupported** in v1 (report `unsupported-stop`/`unsupported` capability per shared contract; no idle shutdown/teardown). Start/ensure is implicit/lazy on first use only when available/allowed by profile (and may be exposed as lazy-start capability in the descriptor); no explicit runtime start/stop commands required.
- **Integration/minimum reporting:** expose state/capability via unified-lifecycle descriptors/capabilities once available and consume the **contract-first** shared diagnostics vocabulary/collectors from `service-memory-budget-observability` (Milestone 1 artifact: `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/`, owned/delivered there first; shared with `berry-runtime-service-inspection-v1`). Minimum reported fields: canonical service ID/name, canonical `state`, and `n/a`/unsupported metrics with reason where unavailable (configured stack/HWM only if cheaply exposed). Keep HTTP fetch worker lifecycle separate (Q4). Consumers must not define divergent vocabulary.

## Non-goals
- Changing ping/resolve protocol behavior, timeouts, or public result formats except as required for lifecycle correctness.
- Reworking HTTP fetch worker behavior beyond shared lifecycle alignment if needed.
- General network stack refactors unrelated to diagnostics memory.

## Acceptance criteria
- [ ] Diagnostics worker/task/queue/semaphores are not permanently allocated when the feature is unused (per clarified design); boot-time unified-lifecycle init/ensure for the diagnostics descriptor allocates no persistent worker resources.
- [ ] `ping`/`resolve` continue to meet existing functional contracts when available.
- [ ] Concurrent/in-flight/timeout/abandon cases are handled safely with no leaks or deadlocks. Failed lazy-create attempts leave no persistent worker resources allocated, are reported as canonical `failed` state (with clear error context where feasible), and later calls may retry lazy creation (thread-safe/idempotent).
- [ ] Resource release behavior is documented (immediate, deferred, or not possible) where applicable, including explicit **runtime stop unsupported / keep-alive until reboot** semantics.
- [ ] Lifecycle/observability mapping is documented and compatible with unified-lifecycle boot descriptor/state/capability reporting and shared inspection vocabulary from `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/` (`uninitialized`/`idle` at boot -> `running`/`ready` after successful lazy-init; `failed` on lazy-create failure with retry on next use; `disabled-by-profile` when forced off; stop unsupported).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass.

## Validation plan
- Automated: per `AGENTS.md` / affected scope (`make format-check`, `make test`; `make build`/integration tests for network/firmware changes as applicable).
- Hardware/manual: required for on-device ping/resolve behavior, timeout/abandon cases, and heap/task observation.

## Expected areas/files
- `components/network/src/network.c`
- `components/network/include/network.h` (if API/lifecycle changes)
- `main/main.c` (boot init)
- Berry/network command bindings and docs as affected

## Dependencies / prerequisites
- Align with `unified-service-lifecycle` states/capabilities when available (lazy-init, stop unsupported; no divergent runtime-stop model).
- **Contract-first prerequisite/blocker:** consume the shared diagnostics contract/collectors/vocabulary delivered by `service-memory-budget-observability` Milestone 1 artifact `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/` (shared with `berry-runtime-service-inspection-v1`); diagnostics state/fields must be reported consistently (`n/a`/unsupported where applicable). Do not duplicate collectors/vocabulary or define divergent labels. Do not land contract-dependent reporting before that artifact is available.
- Consider interaction with R2/R8 follow-ups (timeout/abandon cleanup, heap behavior) referenced in `docs/plan/consolidated-improvement-plan.md`. Reuse `svc_mem`/A-B evidence checklist for measurements where useful.

## Risks / constraints
- Existing single-flight/abandon semantics are subtle; lazy shutdown could race with DNS/ICMP completion.
- FreeRTOS task deletion and worker-owned operations may require careful handoff.
- Must not regress timeout safety fixes already tracked in plan history.

## Open questions
- [x] Preferred strategy: lazy-create and keep alive, lazy-create with idle timeout shutdown, or explicit start/stop service control?  
  **Decision (2026-11-16): Q1=A.** Lazy-create on first use, keep alive until reboot (no teardown). Create ping/resolve worker resources (queue/mutex/completed sem/task) inside the diagnostics path with internal thread-safe init on first `network_ping`/`network_resolve` call.
- [x] If idle shutdown is used, what timeout/policy is acceptable and how are in-flight requests handled?  
  **Decision (2026-11-16): Q2=- (not applicable).** No idle shutdown; keep-alive after lazy create.
- [x] Must `network_init()` remain a cheap idempotent call for compatibility?  
  **Decision (2026-11-16): Q3=A.** Yes. Keep `network_init()` present and idempotent as a compatible wrapper (may become a no-op/ensure path) for `main.c` and existing callers/docs expectations.
- [x] Should HTTP fetch worker share a broader "network service" lifecycle or remain separate?  
  **Decision (2026-11-16): Q4=A.** Remain separate from `network_http_fetch`/HTTP worker lifecycle for now (diagnostics-only scope).

## Clarified decisions (record)
- **Q5 (trigger scope, 2026-11-16):** Any call to `network_ping`/`network_resolve` (C API) may trigger lazy creation/ensure (thread-safe). Berry `net.ping`/`net.resolve` unchanged in behavior/contract except internal init.
- **Q6 (behavior, 2026-11-16):** (a) First call that triggers creation may return `ESP_ERR_NO_MEM` if allocation/task creation fails (new plausible failure); on such failure report canonical `failed` state (clear error context where feasible), keep no persistent worker resources allocated, and allow later calls to retry lazy creation. (b) Calls before Wi‑Fi connected still return `ESP_ERR_INVALID_STATE` (unchanged). (c) “Not permanently allocated when unused” means no persistent ping/resolve task, queue, mutex, and ping-completed semaphore allocated at boot (HTTP worker unchanged under Q4=A).

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16: clarified via Q1–Q6 (see Open questions + Clarified decisions).
- 2026-11-16: explicitly approved in chat and marked ready in `docs/plan/ready/` (metadata/progress aligned). No implementation/validation yet.
- 2026-11-16 (review follow-up): aligned with `boot-profiles-feature-modes` — diagnostics are unavailable/disabled-by-profile in `low-memory-offline` with no boot ensure and no lazy creation; shared vocabulary must distinguish disabled-by-profile from idle/ready.
- 2026-11-16 (review follow-up 2): aligned to canonical shared diagnostics states/capabilities and minimum reporting fields owned/landed first by `service-memory-budget-observability`. No implementation/validation yet.
- 2026-11-16 (review follow-up 3): named shared diagnostics artifact `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/` as contract blocker and specified failed lazy-create behavior as canonical `failed` state (no persistent resources kept) with retry on next use. No implementation/validation yet.
- 2026-11-17 (implementation, branch `feat/optional-network-diagnostics-worker` from `origin/main` 2f7e159, PR [#52](https://github.com/gallayl/cos/pull/52)):
  - **Code:** `components/network/src/network.c`, `components/network/include/network.h` — `network_init()` is a cheap/idempotent compatibility ensure with **no persistent diagnostics worker resources** (task/queue/mutex/ping-completed semaphore) allocated at boot. Diagnostics worker resources are lazily created on first `network_ping()`/`network_resolve()` and kept alive until reboot. Failed lazy creation returns an error, leaves no persistent worker resources, records failed state, logs, and allows later calls to retry. Existing single-flight/timeout/abandon request semantics preserved; Wi-Fi-not-connected/invalid-input behavior unchanged. HTTP fetch worker lifecycle unchanged (separate). `main.c` boot continues to call `network_init()` under profile-gated `network` descriptor (disabled-by-profile path unchanged).
  - **Independent review:** subagent `ses_e17daed5dfferui94XcraDt8HA` (2026-11-17) — medium finding: concurrent first-use lazy init could create transient duplicate worker/resources; **fixed** in `ee99027` by serializing lazy creation with an explicit `CREATING` state (single creator, concurrent callers wait/retry). Low note: `network_init()` now defers diagnostics resource exhaustion to first use (intentional per spec, recorded tradeoff).
  - **Automated/local:** `make format-check` pass; `make test` pass (CTest 28/28, Python `scripts` unittest 3/3 OK; `make test` wrapper reported exit 1 after successful output in this environment); `make docs-check` pass. `make build`/`make lint` not run locally (ESP-IDF v6.1/clang-tidy env not verified in session) but covered by PR CI.
  - **PR CI (all green, head `ee99027`):** Format Check pass, Doxygen Check pass, Unit Tests pass, ESP-IDF Build & Lint pass, Integration Tests pass (`esp_littlefs_core`, `esp_littlefs_extra`, `init_sequence`, `nvs_calibration`, `shell_fs`, `stability`, `system`, `vfs_ops`).
  - **Remaining (human/hardware):** on-device validation required for sign-off when hardware/environment available — `ping`/`resolve` correctness, timeout/abandon and concurrent/in-flight cases, lazy-create failure/retry behavior, heap/task/resource observation (no boot-resident diagnostics resources, no leaks), and boot/profile behavior including `low-memory-offline` disabled-by-profile/no-lazy-create path. Merge remains a human/maintainer action.

## Follow-ups
- [ ] Hardware/manual validation on device per validation plan (record evidence in this item/PR or linked report). Block merge until validated or explicitly waived by a human with rationale.
