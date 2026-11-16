# Optional network diagnostics worker lifecycle

- **ID:** optional-network-diagnostics-worker
- **Status:** ready
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:135`, `components/network/src/network.c:18-24`, `components/network/src/network.c:281-325`.
- **Approval:** approved (2026-11-16)

## Summary
Network diagnostics (`ping`/`resolve` worker) are initialized at boot and always create synchronization objects plus a dedicated task/stack. This reserves RAM even when diagnostics are never used. The feature should make this worker optional or on-demand with clear cleanup behavior.

## Scope
- Make diagnostics worker resources created lazily on first use and/or controllable via service lifecycle.
- Define cleanup/stop behavior for queue, semaphores, task, and in-flight/abandoned requests.
- Preserve existing `network_ping`/`network_resolve` semantics, timeout behavior, and single-flight constraints unless explicitly changed.

## Non-goals
- Changing ping/resolve protocol behavior, timeouts, or public result formats except as required for lifecycle correctness.
- Reworking HTTP fetch worker behavior beyond shared lifecycle alignment if needed.
- General network stack refactors unrelated to diagnostics memory.

## Acceptance criteria
- [ ] Diagnostics worker/task/queue/semaphores are not permanently allocated when the feature is unused (per clarified design).
- [ ] `ping`/`resolve` continue to meet existing functional contracts when available.
- [ ] Concurrent/in-flight/timeout/abandon cases are handled safely with no leaks or deadlocks.
- [ ] Resource release behavior is documented (immediate, deferred, or not possible) where applicable.
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
- Should align with `unified-service-lifecycle` if a common service model is adopted.
- Consider interaction with R2/R8 follow-ups (timeout/abandon cleanup, heap behavior) referenced in `docs/plan/consolidated-improvement-plan.md`.

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
- **Q6 (behavior, 2026-11-16):** (a) First call that triggers creation may return `ESP_ERR_NO_MEM` if allocation/task creation fails (new plausible failure). (b) Calls before Wi‑Fi connected still return `ESP_ERR_INVALID_STATE` (unchanged). (c) “Not permanently allocated when unused” means no persistent ping/resolve task, queue, mutex, and ping-completed semaphore allocated at boot (HTTP worker unchanged under Q4=A).

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16: clarified via Q1–Q6 (see Open questions + Clarified decisions).
- 2026-11-16: explicitly approved in chat and marked ready in `docs/plan/ready/` (metadata/progress aligned). No implementation/validation yet.
