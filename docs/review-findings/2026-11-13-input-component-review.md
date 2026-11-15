# Input component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/input/` (`include/keyboard_event.h`, `include/keyboard_router.h`, `src/keyboard_router.c`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, and maintainability of the `input` component as-is (senior-dev breakage review).  
**Load assumed:** Single device user; keyboard/key events arrive occasionally (human key press/release rate), routed on the caller's context (not many concurrent producers). Wired in `main/main.c` to route events to console prompt, Berry GUI, or shell input.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| — | — | No defects found in this audit | — | — | Revisit if routing rules or event producers change |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

No Must fix / Should fix / Nice to have findings.

- Routing order and conditions match the stated contract and existing host tests (`test/test_keyboard_router.c`): `NULL` event/router ignored; console ownership sends key-down text to shell and skips GUI; otherwise GUI may consume press/release; unconsumed key-down text goes to shell, releases do not.
- Callbacks and `context` are checked/forwarded safely for `NULL` callback pointers. No dynamic allocation, shared mutable state, or blocking work in the router.

## Validation / not checked

- Behavior on real hardware with the Bluetooth HID producer (`bluetooth_hid_set_key_event_callback` path in `main/main.c`) and actual key repeat/hold patterns.
- Thread/concurrency safety if `keyboard_route_event` is ever called from multiple tasks/ISRs concurrently (current design has no internal state; safety depends on caller/callbacks).
- Whether producers ever set `event->text_len` larger than `sizeof(event->text)`; router passes `text`/`text_len` through to `shell_input` without clamping (contract relies on a valid `keyboard_event_t`).
- ESP-IDF device build/integration beyond host-side reasoning (run relevant checks per `AGENTS.md` when changing this component).

## Related code/tests

- Implementation: `components/input/src/keyboard_router.c`, `components/input/include/keyboard_router.h`, `components/input/include/keyboard_event.h`, `components/input/CMakeLists.txt`
- Existing host tests: `test/test_keyboard_router.c` (registered in `test/CMakeLists.txt`)
- Main integration/callers: `main/main.c` (`route_keyboard_event`, `S_KEYBOARD_ROUTER`, `bluetooth_hid_set_key_event_callback`)
