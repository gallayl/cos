# R18 — `async`/`await` runtime design + implementation

- **ID:** r18-async-await-runtime
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Ready:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R18, P2), [`history/2026-11-07-ts-feature-extensions.md`](../../history/2026-11-07-ts-feature-extensions.md), clarified from `needs-clarification/r18-async-await-runtime.md` (history preserved in git)
- **Approval:** User explicitly approved marking Ready/pre-approved for implementation+PR flow in chat on 2026-11-15 ("Approve and mark Ready (Recommended)"). Merge remains human/maintainer action.

## Summary
Design-first effort: define concurrency model on constrained hardware (Berry VM single-threading/tasking, interaction with timers/network callbacks, cancellation, memory/resource lifetime) before any compiler lowering. No silent desugaring.

## Decisions (recorded)
- **2026-11-15 (user): Concurrency/execution model:** **Cooperative single-VM scheduler.** Cooperative task/promise scheduler in the Berry runtime/event loop: `await` yields to pending microtasks/timers/callback completions on the same VM thread (no concurrent VM access). Fits callback/timer/network patterns and R24’s serialized bridge; requires explicit yield points and bounded queues.
- **2026-11-15 (user): Scheduler pump/integration:** **Runtime pump hooked into existing main/UI loop ticks.** Add `berry_runtime_pump()`/equivalent called from existing periodic points (main loop/UI tick/timer service callbacks as appropriate) so due timers/promises progress without blocking/busy-wait. Bounded queues enforced; overflow fails fast with clear logged error (no silent drop, no VM corruption).
- **2026-11-15 (user): v1 milestone scope:** **Minimal.** Support `async function`, `await`, `throw`/try-catch integration, and a minimal Promise/awaitable core. **First awaitable:** timer/sleep only (delay/promise) to validate scheduler/lifecycle/memory before networking/UI APIs. **Out of v1:** cancellation API/syntax, `for await`, `Promise.all/race`/combinators/`finally`, net.fetch/UI awaitables, async R24 handlers.
- **2026-11-15 (user): Promise/awaitable contract:** **JS-like minimal core + log unhandled rejections.** `Promise` with `new Promise(executor)` (executor runs synchronously), `Promise.resolve`, `Promise.reject`, instance `then(onFulfilled?, onRejected?)`, `catch(onRejected)`. One-way settlement; `then/catch` callbacks enqueued as microtasks/ready jobs drained cooperatively by scheduler pump on the VM thread. `await` consumes Promises/thenables per this contract. **Unhandled rejection:** log error with context (no VM crash by default). No `finally`/combinators in v1.
- **2026-11-15 (user): v1 limits (defaults, spec-recorded; tunable with evidence):** **Small fixed caps.** Defaults: max **16** pending timers, max **32** queued microtasks/ready callbacks per drain cycle, max **16** tracked async tasks/promises per app context (exact constants documented/validated on hardware). **Overflow:** timer scheduling returns/rejects with clear error (and logs); queue overflow fails fast with logged error and prevents VM corruption (no silent drop).
- **2026-11-15 (user): Entrypoints/lifecycle:** **Entrypoints stay sync; may spawn tracked async work.** `run(args)`/`__cos_entry`/command `main(args)` remain synchronous in v1. They may start async operations (timers/promises) that are tracked and cleaned up on app close/VM reset (per R13/R21). Async completion does not extend app lifetime in v1; behavior when work is pending at exit follows teardown rules.
- **2026-11-15 (user): Teardown/cleanup:** **Cancel timers + settle pending as cancelled, stop callbacks.** On app close / `berry close` / VM reset: stop pumping for the context, cancel/remove all pending timers and release resources, reject unsettled tracked promises with a deterministic cancellation reason (tagged/cancellation error value) and guarantee **no user callbacks run after teardown completes**. Unhandled cancellation rejections handled consistently with unhandled-rejection logging policy (document precise semantics).
- **2026-11-15 (user): Budgets & evidence:** **R5-style checkpoints + fixed limits + hardware timing.** Document per-promise/task overhead estimates, queue sizes, max pending timers/tasks, stack headroom for compiler-generated state machines, and no busy-wait. Require R5-style heap/watermark checkpoints and leak/teardown tests (repeated start/close/VM reset), plus hardware validation for timer accuracy/latency and stability under load. Block merge if budgets exceeded without mitigation.
- **2026-11-15 (user): R24 interaction:** R24 (`@cos/http` v1) handlers remain **serialized sync-completion in v1**. R24 v1 may proceed independently in the ready queue, but R18 completion triggers a **required compatibility review** of R24 for async handlers/non-blocking patterns/timeouts/cancellation before treating R24’s handler model as final long-term.

## Open questions
- None.

## Cross-references / review triggers
- [ ] **Review R24 (`@cos/http` v1) when R18 lands:** R24 (`docs/plan/ready/r24-berry-script-http-server-api.md`) specifies a serialized, single-request-at-a-time sync-completion bridge with handler timeouts and no async/await handler model. Once R18 async/await runtime/compiler support is designed/implemented, review R24 for async handlers, non-blocking I/O patterns, cancellation/timeout semantics, and concurrency-model compatibility. **v1 decision (2026-11-15):** R24 remains sync in R18 v1.

## Scope and non-goals
- **Scope (v1):** Document and implement a minimal, Berry-compatible `async`/`await` design for ESP32/single Berry VM: cooperative scheduler with runtime pump hooked into existing main/UI loop ticks (no busy-wait, no concurrent VM access), compiler lowering/runtime for `async function`/`await` with `throw`/try-catch integration, minimal JS-like Promise core (`new Promise` sync executor, `Promise.resolve/reject`, `then`/`catch`, microtask/ready-job queue drained on VM thread), **timer/sleep awaitable only**, bounded queues/limits (defaults: 16 pending timers, 32 queued microtasks/ready callbacks per drain cycle, 16 tracked async tasks/promises per app context; overflow errors/fail-fast, no silent drop), app-owned tracking, and deterministic teardown (cancel timers/release resources, reject unsettled tracked promises with cancellation reason, no user callbacks after teardown). Entrypoints (`run(args)`/`__cos_entry`/`main(args)`) remain sync and may spawn tracked async work that does not extend app lifetime. Unhandled rejections (incl. cancellation per policy) logged with context, no VM crash by default. R24 remains sync in v1.
- **Non-goals (v1):** silent desugaring, FreeRTOS/background-task-per-async or any concurrent Berry VM access, `for await`, `Promise.finally`/`all`/`race`/other combinators, user-facing cancellation API/syntax, net.fetch/UI/callback async APIs, async R24 handlers, awaiting app completion from async entrypoint return. Future milestones: extend awaitables/APIs, cancellation API, and review R24/R18 integration.

## Acceptance criteria
- [ ] Runtime/concurrency design documented and approved (scheduler pump integration, Promise/awaitable contract, microtask semantics, lifecycle/teardown, limits/overflow, budgets/measurements, R24 interaction).
- [ ] Compiler lowering for `async function`/`await` implemented per approved design with diagnostics for unsupported constructs (no silent desugaring/miscompile).
- [ ] Minimal runtime/firmware support implemented: cooperative scheduler + pump hooks, timer/sleep awaitable, Promise core, app-owned tracking, deterministic teardown/cleanup.
- [ ] Automated tests added/updated (frontend/compiler; host/unit where feasible) covering success/error paths, teardown/cleanup, and limit/overflow behavior.
- [ ] Evidence recorded: exact commands/results for required checks per `AGENTS.md`, plus R5-style heap/watermark/leak checks (repeated start/close/VM reset) and hardware validation for timer accuracy/latency/stability under load; budgets/limits met or mitigated with evidence.

## Validation plan
- **Automated (per scope):** `make format-check`, `make test`; frontend changes: from `frontend/` `yarn format:check`, `yarn lint`, `yarn test`, `yarn build`; firmware/compiler/runtime: `make build` and `make lint` when feasible (ESP-IDF v6.1 env), `make docs-check` if public headers/Doxygen affected, integration/QEMU as applicable.
- **Hardware/manual (required):** on supported hardware validate timer/sleep awaitable timing/latency, scheduler progress via runtime pump, repeated run/close/VM-reset teardown with no leaks/dangling callbacks (R5-style checkpoints/watermarks), behavior at/over limits, and no regression to existing Berry/TS runtime behavior.

## Expected areas/files (rough)
- `frontend/src/ide/berry/` (AST/compiler lowering, diagnostics, tests), `frontend/src/cos-sdk/` and related TS types if Promise/timer APIs exposed.
- `components/berry_lang/src/` (Berry runtime/scheduler/pump, Promise/timer bindings, app/task tracking, teardown hooks), related headers/includes.
- `main/` or runtime integration points for periodic pump hook if needed; existing app lifecycle/VM reset paths (`berry run/exec/close`).
- `storage/ts/` + `storage/flash_data/` examples/tests if script-facing APIs/examples added (with compiled `.be` artifacts per repo rules).
- `test/`, `frontend/` tests, `docs/reports/` for hardware/runtime evidence.

## Dependencies / prerequisites
- Pinned Berry semantics and existing runtime/lifecycle contracts (R13/R21, R5 memory practices, R1/R6 module/loading behavior).
- Existing timer/event-loop facilities suitable for cooperative pump integration on ESP32 (investigate during implementation).
- R24 remains sync in v1; review/align if R18 implementation exposes constraints (cross-reference).
- **Coordination (2026-11-16 review):** overlaps with other ready work touching Berry runtime/shell/SDK surfaces (`r20-pipes-runtime-v1`, `r22-m4-time-read-only`, `berry-runtime-service-inspection-v1`). No strict implementation order required, but coordinate APIs/hooks/teardown behavior and run relevant shared runtime/shell/SDK validation when changes land close together.

## Risks / constraints
- **Memory/stack pressure:** state machines/promises/queues consume constrained RAM/stack; enforce fixed limits and require R5-style evidence.
- **VM/thread safety:** all Berry VM access must remain on single thread; no callbacks after teardown.
- **Timer accuracy/latency:** cooperative pumping depends on loop tick cadence; validate on hardware.
- **Unhandled rejections/teardown semantics:** must be deterministic and observable (logging) to avoid silent failures/leaks.
- **Scope creep:** networking/UI awaitables and cancellation add significant complexity (R2/R8/R24 interactions); explicitly deferred.

## Progress / evidence
- 2026-11-15: created (design required before implementation).
- 2026-11-15: added cross-reference to R24 (`docs/plan/ready/r24-berry-script-http-server-api.md`) — review R24’s sync/serialized handler model when async/await is designed/implemented.
- 2026-11-15: clarification pass 1 (user): cooperative single-VM scheduler, minimal v1 (async/await + Promise-like core + timer/sleep only), app-owned tasks cleaned on close/VM reset, R24 sync in v1.
- 2026-11-15: clarification pass 2 (user): runtime pump via existing loop ticks, JS-like minimal Promise core + log unhandled rejections, sync entrypoints may spawn tracked async work, R5-style budgets+fixed limits+hardware timing, small fixed caps (16 timers / 32 microtasks per drain / 16 tracked per app context) with fail-fast overflow, teardown cancels timers + rejects unsettled as cancelled + no callbacks after teardown. Technical questions resolved.
- 2026-11-15: **Approved Ready** by user in chat; moved to `docs/plan/ready/r18-async-await-runtime.md` (pre-approved for implementation/PR flow; merge remains human). Implementation/evidence pending.
