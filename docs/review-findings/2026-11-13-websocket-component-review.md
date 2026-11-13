# WebSocket component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/websocket/` (`include/websocket.h`, `src/websocket.c`, `CMakeLists.txt`), with usage traced in `main/main.c` and integration points in `components/http_server/` (`include/http_server.h`, `src/http_server.c`).  
**Reviewer focus:** correctness, safety, concurrency, resource handling, and fit for current use.

## What this repo does

COS is ESP32 (CYD/ESP32-2432S028R) firmware with a display, local input, and serial shell. `components/websocket` adds a `/ws` WebSocket endpoint on the device HTTP server for the browser console: after HTTP Basic auth, a client is tracked by socket, console output is broadcast to connected clients, and incoming text frames are turned into shell input through a callback.

**Load assumed:** one local user/device owner, a small number of browser clients at once (HTTP server allows 4 client slots; websocket tracks up to 4 clients), occasional commands and console output, not many tasks changing websocket state at runtime.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| W1 | Must fix | Oversized WebSocket frame is rejected without reading its payload | Open |  | Can leave unread bytes on the connection and break later frames |
| W2 | Should fix | `websocket_init` leaks mutex and leaves bad state on early failure | Fixed |  | 2026-11-13: `components/websocket/src/websocket.c` now checks for HTTP server before creating `s_mutex` and avoids overwriting an existing mutex on init. |
| W3 | Should fix | Text handler and context are shared without synchronization | Open |  | `ws_handler` reads while `websocket_set_text_handler` can write from another task |
| W4 | Nice to have | Single global session-close callback slot | Open |  | `http_server_set_session_close_cb` holds one callback; websocket owns it exclusively |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### Must fix

1. **Oversized frame is dropped without reading it** (`components/websocket/src/websocket.c:L165-L181`)
   - **What this is:** For each incoming WebSocket message, `ws_handler` first reads frame info (type and length), then allocates a buffer and reads the payload bytes.
   - **Problem:** If the frame is larger than `WEBSOCKET_MAX_TEXT` (126 bytes), the code logs and returns `ESP_ERR_INVALID_SIZE` without reading those payload bytes. A client that sends a 200-byte text frame leaves 200 bytes unread on that socket.
   - **Fix:** When `frame.len` is too large (or when payload cannot be handled), read and discard the payload in a small loop/buffer, then close or cleanly ignore that message before returning success. Keep the max accepted shell line the same.
   - **If we skip it:** One long message can corrupt that connection's next reads, so later commands or control frames may fail until the client reconnects.

### Should fix

2. **Init can leak its mutex on failure** (`components/websocket/src/websocket.c:L232-L245`)
   - **What this is:** `websocket_init` creates `s_mutex`, then checks that the HTTP server exists.
   - **Problem:** If the server is not running, it returns `ESP_ERR_INVALID_STATE` but does not delete `s_mutex` or clear it. A later successful init would also overwrite `s_mutex` with a new mutex. This is easy to hit in a unit/integration setup that retries init or calls it in the wrong order.
   - **Fix:** Check the HTTP server before creating resources, or delete/clear `s_mutex` on every early return. If re-init is not supported, make it fail clearly without changing global state.
   - **If we skip it:** Failed or repeated startup can leak a mutex and leave websocket globals in a confusing state, which makes tests and recovery harder to reason about.

3. **Incoming text callback is not synchronized** (`components/websocket/src/websocket.c:L20-L27`, `components/websocket/src/websocket.c:L205-L219`)
   - **What this is:** `websocket_set_text_handler` stores the function pointer and context in module globals; `ws_handler` reads them on the HTTP server task when a text frame arrives.
   - **Problem:** If the handler is changed while a client sends text, one task can write `s_text_handler` and `s_text_handler_context` while another task reads them, so the call can pair a new function with an old context. Today `main` sets the handler once after init, but the API is public.
   - **Fix:** Copy handler and context into local variables under the mutex (or use one atomic snapshot pattern) before calling, and take the same mutex in the setter. Keep the handler call outside the lock if it may block.
   - **If we skip it:** At current startup-only use this is unlikely, but a future runtime change can send shell input to the wrong context or crash.

### Nice to have

4. **Session-close callback is a single global slot** (`components/http_server/src/http_server.c:L11-L26`, `components/websocket/src/websocket.c:L247`)
   - **What this is:** The HTTP server stores one `session_close_cb` and websocket replaces it during init.
   - **Problem:** Any other component that needs socket-close cleanup cannot register its own callback without removing websocket cleanup. There is no second user in the current tree, so this is not a bug now.
   - **Fix:** Only if another user appears: allow multiple listeners or let the server own a small notifier. Otherwise keep the current simple design.
   - **If we skip it:** No effect today. It becomes a small integration trap only when a second per-socket feature is added.

## Verdict

Usable for the current single-user console bridge, but fix oversized-frame handling first because one bad client message can break that WebSocket connection.

## Lean

`Lean: -0 lines, -0 dependencies possible.` (I did not find clear dead code or an unused dependency in this small module.)

## Not checked

- 2026-11-13: Ran `make format-check` and `make test` after W2 fix — **pass** (host checks). `components/websocket` has no dedicated host unit tests in `test/`.
- On-device ESP-IDF build, runtime behavior, and real browser WebSocket traffic (handshake, auth failures, disconnects, binary/fragmented/ping-close frames) were not exercised in this review.
- ESP-IDF `httpd_ws_recv_frame`/`httpd_ws_send_data_async` semantics under partial reads, send backpressure, or socket reuse beyond static reasoning.
- Concurrency/load beyond the assumed small-client, occasional-use case (many reconnects, rapid output bursts, or handler changes while traffic is active).
- HTTP auth and static/file/API handlers in `components/http_server` except the session-close and handler-registration integration points used by websocket.
