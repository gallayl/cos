# R24 — Berry/TS callback-based HTTP server API (v1, simple chat)

- **ID:** r24-berry-script-http-server-api
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Ready:** 2026-11-15
- **Source/links:** [`ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (listed as R23 “Simple callback-based HTTP server API for Berry/TS…”), clarified from [`needs-clarification/r24-berry-script-http-server-api.md`](../needs-clarification/r24-berry-script-http-server-api.md) (history preserved in git), `components/http_server/*`, `components/berry_lang/src/`, `frontend/src/cos-sdk/`, `frontend/src/ide/cos-binding-inventory.ts`
- **Approval:** User approved specification/recommendations in chat on 2026-11-15 (“approve”). **Pre-approved for implementation/PR/babysit-to-green flow** (merge remains human/maintainer action). Note: R24 v1 may proceed independently, but once **R18 (`async`/`await`)** is implemented this feature/spec must be **reviewed** for async handler/concurrency implications before treating R24’s handler model as final long-term (see [`r18-async-await-runtime.md`](./r18-async-await-runtime.md), ready/design-approved 2026-11-15).

## Summary
Deliver a v1 script-facing HTTP server API in module `@cos/http` for Berry/TS: a **separate user server instance** (not the core device HTTP server) on a user-defined, non-core port, supporting Express-like routing/basic middleware and request/response helpers for a **simple chat** use case, with safe serialized callback execution, single-request-at-a-time handling, dynamic route management, and no built-in auth.

**Distinction (resolved, 2026-11-16 review):** this is **not** the core device HTTP/WS service controlled by [`./optional-http-websocket-service.md`](./optional-http-websocket-service.md). R24 creates an independent user `@cos/http` listener/resources on a non-core port and must not be started/stopped/coupled via core `server`/`ws` lifecycle controls except normal port/conflict rules.

## Open questions
- None. (Future: review against R18 async/await when available — tracked as cross-reference, not a blocker for this v1 spec.)

## Scope (v1)

- **Module:** `@cos/http` (Berry module + TS SDK declarations + IDE/binding inventory/manifest registration).
- **Separate instance:** `createServer()` creates an independent user HTTP server instance (own `httpd`/listener/resources), **not** sharing or modifying core device routes/handlers/auth/static/WS.
- **Listen/port:** `listen(port, host?)` binds a user-specified port. Port **must not** overlap/conflict with the core device HTTP server port or any in-use port; on collision/bind failure **return/throw a clear error** (e.g. mapped `EADDRINUSE`/invalid-state, message like “port in use or conflicts with core server”) with **no partial listener/start**. Default bind/host: **`0.0.0.0`** (LAN-reachable) unless explicitly set (e.g. `127.0.0.1` allowed if provided). Port must be explicit and valid/non-core.
- **Routing & middleware:** register handlers for HTTP methods **GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS** and `all/any` (as specified) plus **basic middleware** via `use(...)`. v1 matching supports **exact paths** and **parameterized segments** (e.g. `/chat/:room`) with `req.params`; query string parsed into `req.query` (key -> string; repeated keys use first value unless trivial to support array — define deterministically in implementation). No regex routing required in v1.
- **Request object (v1):** provides `method`, `url`/`path`, `params`, `query`, `headers`, and body access (`body` raw/buffer-or-string as appropriate, plus helpers `text()` and/or `json()` as specified) within size limits.
- **Response object (v1):** supports `status(code)`, `set(name,value)`/`header(...)`, `send(body)`, `json(value)`, `end([body])` (and `type(...)` optional if trivial). Streaming/chunked bodies **out of scope for v1** unless required by implementation simplicity (default: buffered send/end).
- **Dynamic routes:** routes/middleware may be **registered and unregistered at runtime** (`remove`/equivalent API provided). Mutations are **serialized/thread-safe** with request handling; to keep v1 simple, mutations are allowed from safe Berry context and **not during an in-flight request dispatch** (if attempted, return/throw a clear error or defer deterministically — behavior specified and tested).
- **Execution model:** **safe and robust, single request at a time**. Incoming requests are handled via a **serialized bridge** (no concurrent Berry VM access): request dispatched to Berry runtime/context, httpd side waits for completion with a **handler timeout** (default **5000 ms**, bounded/configurable within safe limits), then response is sent. If busy, reject with **503** (or equivalent) per limits.
- **Blocking/long ops:** handlers may run synchronous script logic and call safe APIs but **must complete within timeout**. Long/blocking operations (e.g. `net.fetch`, long sleeps, heavy filesystem) are **not recommended in v1** (documented risk under single slot); timeout still enforced. No response/timeout -> **504** (and log); server remains healthy.
- **Lifecycle & cleanup:** `close()` stops listener, unregisters routes/middleware/handlers, aborts/closes outstanding connections, frees resources (idempotent). On script/app stop or Berry VM reset/teardown, any owned user-server instance is **auto-cleaned** (no dangling callbacks/leaks).
- **Error/edge behavior (v1):** handler exception/uncaught error -> log + **500** (generic body); timeout/no response -> **504**; double-send/write-after-end -> safe no-op or logged error (no stream corruption); client disconnect -> abort/cleanup; invalid route/method -> **404/405**; body/limit/parse overflow or malformed request -> **400/413/431** as applicable; listen collision/core-port conflict -> explicit error, no partial start.
- **Security:** **no built-in auth** (apps implement their own). Separate LAN-reachable service (default `0.0.0.0`), isolated from core APIs/port, **no HTTPS in v1**. Docs must warn: app responsible for auth/input validation; do not expose sensitive functionality without protection.
- **Limits (v1 defaults, conservative/finalized in implementation within ESP32 constraints):** single in-flight request; max URL **512 B**; headers **<=16**, **<=4 KB** total; body **<=4 KB**; routes+middleware **<=16**; handler timeout **5000 ms** (bounded). Supported request bodies: **text/raw and application/json**; unsupported/oversize/malformed -> 4xx per above. User server uses its own `httpd` config sized conservatively (small sockets, measured stack).

## Non-goals (v1)
- Coexistence/override of core device routes, core port, core auth/static/SPA/WS behavior.
- Built-in authentication/authorization, TLS/HTTPS, HTTP/2.
- Streaming/chunked request/response bodies (buffered only in v1) unless trivially implied.
- Concurrent request handling beyond single-at-a-time; WebSocket integration in script API.
- Regex/advanced routing beyond exact + `:param` segments; multipart/form-data parsing unless added explicitly.
- Async/await handler model (pending R18) — v1 uses serialized sync-completion-with-timeout bridge. **Handler API is synchronous in v1; do not design handler signatures that assume promise/async returns** (review required under R18 before changing).

## Acceptance criteria
- [ ] `@cos/http` available in Berry and TypeScript SDK (declarations + IDE/binding inventory/manifest) with documented v1 API/signatures and limits.
- [ ] Separate user server instance can `listen(port, host?)` on non-core port and serve requests; **port collision/conflict with core or in-use port returns/throws clear error** with no partial start; `close()` is idempotent and releases resources.
- [ ] Routing/methods/middleware/params/query and req/res helpers work per spec (exact + `:param`, GET/POST/PUT/DELETE/PATCH/HEAD/OPTIONS/all as implemented), with JSON/text body parsing and size/parse enforcement (4xx/413/431 as specified).
- [ ] **Single request at a time** enforced; handler timeout/exception/timeout-no-response/double-send/disconnect behaviors match spec (500/504/safe handling); server remains healthy after errors.
- [ ] **Dynamic register/unregister** works per spec (serialized, safe contexts) and **auto-cleanup** occurs on `close()`, script/app stop, and Berry VM reset/teardown (no dangling callbacks/leaks).
- [ ] **Simple chat example** (Berry/TS, in-memory, bounded): `POST /send` accepts JSON message and stores it; `GET /messages` returns recent messages as JSON (minimal protocol documented in example). Runs on non-core port.
- [ ] **Echo JSON** scenario works: POST JSON body to an endpoint returns equivalent JSON response (status/body/headers as specified).
- [ ] **Unit tests with good coverage** for route matching/params/query, request parsing/body & limit enforcement, response helpers/status/headers, middleware chain/`next`, register/unregister/close lifecycle, port collision/error cases, timeout/exception/double-send/no-response handling (host/mocked where feasible).
- [ ] **Integration test** for echo JSON added if feasible in repo test environment (e.g. `integration_test/`/QEMU); if not feasible, limitation documented with rationale and compensating host/on-device evidence.
- [ ] On-device hardware validation evidence recorded: start server on non-core port, simple chat + echo JSON, port-collision error, single-request behavior, dynamic route changes, and cleanup on stop/reset (and no regression to core HTTP server/APIs).
- [ ] Docs updated (usage, limits, LAN/no-auth warnings, example references). No regression to existing HTTP server/core APIs; automated checks pass per `AGENTS.md` for affected scope.

## Validation plan
- **Automated (required per scope):** `make format-check`, `make test`. Firmware: `make build` and `make lint` when feasible (ESP-IDF v6.1 env). Frontend: from `frontend/` run `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` if `frontend/src/cos-sdk/` or IDE bindings changed. Docs: `make docs-check` if Doxygen/public headers affected. Integration: add/run echo JSON integration test if feasible (`make integration-test-build` / `make integration-test`) — document if environment/feasibility blocks.
- **On-device/hardware (required):** validate on supported hardware (Wi‑Fi/LAN client): user server on non-core port; simple chat flow; echo JSON; port collision with core/in-use port; single-request/timeout behavior as applicable; dynamic register/unregister; cleanup on close and on script/app stop or VM reset; confirm core device HTTP server/APIs unaffected. Record firmware/commit, steps, and results in a report linked from this item and consolidated plan.

## Expected areas/files
- `components/http_server/` (user-server instance, routing, bridging, lifecycle/limits) and/or dedicated component as appropriate
- `components/berry_lang/src/` (new `@cos/http` module/bindings) and `components/berry_lang`/runtime integration for safe dispatch/cleanup
- `frontend/src/cos-sdk/http.d.ts` (or equivalent) and exports; `frontend/src/ide/cos-binding-inventory.ts`, manifest/SDK registration as required
- `storage/ts/` examples/apps (chat, echo) and compiled `storage/flash_data/` artifacts as applicable
- `test/` (host unit tests, mocks/seams as needed), `integration_test/` (echo JSON if feasible), `docs/reports/` (HW validation evidence)

## Dependencies / prerequisites
- ESP-IDF `esp_http_server`/LWIP capabilities and existing `http_server` component coexistence.
- Berry runtime support for serialized callback dispatch and lifecycle hooks/cleanup on app stop/VM reset (implement as part of this work, safely).
- **Cross-reference:** R18 `async`/`await` (ready/design-approved 2026-11-15, implementation not started) — when R18 is implemented, review this API for async handlers/non-blocking patterns, timeout/cancellation semantics, and concurrency model compatibility ([`r18-async-await-runtime.md`](./r18-async-await-runtime.md)).

## Risks / constraints
- Berry VM thread-safety and dangling callbacks across VM reset/app stop — mitigated by serialized dispatch and mandatory auto-cleanup.
- ESP32 memory/stack/socket/handler limits; single in-flight serialization means slow handlers reduce throughput — documented (handler timeout, avoid long blocking ops in v1).
- LAN exposure without built-in auth — isolated port and explicit docs/warnings; app responsibility.
- Integration feasibility in QEMU/networked scenarios may vary — document limitations and rely on host + hardware evidence per `AGENTS.md`.

## Progress / evidence
- 2026-11-15: clarified from idea and user decisions (passes 1–3), recommendations approved by user in chat; promoted to `docs/plan/ready/` as pre-approved for implementation/PR/babysit-to-green (merge remains human). R18 async/await review cross-referenced.
