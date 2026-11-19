# Logging levels, command visibility, and log statement audit

- **ID:** logging-levels-and-audit
- **Status:** ready (approved)
- **Created:** 2026-11-19
- **Source/links:** Ponytail ultra logging review (session 2026-11-19). Related code: `components/text_console/*`, `main/main.c`, `components/*/src/*.c|.cpp` (`esp_log.h` usage). Cross-links: `docs/plan/consolidated-improvement-plan.md` (2026-11-19 logging review follow-ups), `docs/plan/needs-clarification/esp-littlefs-stability-hardening.md` (vendored `esp_littlefs` log noise is out of scope here; coordinate if needed).
- **Approval:** Approved by user in chat (2026-11-19): “Go to Ready” (explicit approval after specification). Merge remains a human action.

## Summary
Standardize and clarify ESP-IDF logging behavior on device: global level configured/persisted via `text_console` (NVS), user-facing levels exposed by the `logging` command/API including **debug** (plus existing error/warn/info), output routing described consistently (UART/serial mirrored to the on-screen text console, with optional WebSocket broadcast noted where relevant), and a short logging policy applied to app `ESP_LOG*` usage (demote/remove noisy cases; document policy). Vendored `esp_littlefs` `ESP_LOGV` noise is out of scope for this item.

## Scope
- Document and implement consistent global log level control (NVS-backed via `text_console`, applied at startup). Default remains **WARN** after factory/reset/invalid NVS (unchanged unless code changes require explicit note).
- Expose user-facing levels via `logging` command/API: **error, warn, info, debug** (maps to `ESP_LOG_ERROR/WARN/INFO/DEBUG`). **Verbose** is out of scope for this item unless separately approved.
- Apply a short logging policy to app/component code: audit notable `ESP_LOGD`/severity choices; **demote or remove noisy/unhelpful messages** and document the policy (what goes to INFO vs DEBUG vs WARN/ERROR). Examples to review include (non-exhaustive): `components/bluetooth/src/bluetooth.c`, `components/wifi/src/wifi_cmd.c`, `components/websocket/src/websocket.c`, `components/http_server/src/http_server_cmd.c`, `components/filesystem/src/vfs.c`, `components/filesystem/src/flash.c`.
- Clarify output routing in docs/UI/help text: **“UART/serial output and on-screen text console (mirrored stream)”**; mention optional WebSocket broadcast listener where relevant (e.g. `main.c`/text console listener behavior) without redesigning that path.
- Cross-link `docs/plan/needs-clarification/esp-littlefs-stability-hardening.md` for vendored `esp_littlefs` `ESP_LOGV` noise (out of scope here).

## Non-goals
- Redesigning logging (structured logs, per-tag UI/editors, new sinks beyond existing UART/console/WS listener paths) unless explicitly approved later.
- Large refactors unrelated to level policy and the agreed log-statement cleanup.
- Changing vendored third-party logging behavior by default in this item (handled via the esp_littlefs hardening item if pursued).
- Adding `verbose`/`ESP_LOGV` exposure now.

## Acceptance criteria
- [ ] Source of truth documented: library (`esp_log.h`), persistence (`console` NVS namespace/key via `text_console`), default level (**WARN** on reset/invalid/missing), and mapping to `ESP_LOG*` including `debug` → `ESP_LOG_DEBUG`.
- [ ] `logging` command help/usage and `text_console` API/docs/types/enums aligned to support **error|warn|info|debug** (and do not claim unsupported levels). Validation rejects invalid levels; persistence and startup apply behavior are consistent.
- [ ] Short logging policy documented (where to use ERROR/WARN/INFO/DEBUG) and applied: noisy/unhelpful app log lines are demoted or removed; reviewed inventory from the 2026-11-19 review is addressed with recorded rationale where anything kept.
- [ ] Output routing terminology updated to **“UART/serial output and on-screen text console (mirrored stream)”** (and optional WS broadcast noted where appropriate) in user-facing/docs/help text touched by this work.
- [ ] Vendored `esp_littlefs` `ESP_LOGV` called out as out of scope with a link to `docs/plan/needs-clarification/esp-littlefs-stability-hardening.md`.
- [ ] Validation evidence recorded per `AGENTS.md` scope for any code changes (format/test/build/lint/frontend as applicable); if on-device behavior changes, include a short hardware smoke (set each supported level, reboot, persistence, representative visibility) or state “none”.

## Validation plan
- Automated: per `AGENTS.md`. Minimum for firmware/host changes: `make format-check`, `make test`. Run `make build`/`make lint` when ESP-IDF v6.1 environment available (record blockers if not). No frontend changes expected unless docs tooling requires it; if touched, run frontend checks in `frontend/`.
- Hardware/manual (expected if command/API/persistence/visible behavior changes): brief smoke on device — set `error|warn|info|debug`, reboot, confirm persisted level restored, verify representative logs appear/disappear appropriately. Record commands/outcomes; otherwise state “none”.

## Expected areas/files
- `components/text_console/include/text_console.h`, `components/text_console/src/text_console.cpp`, `components/text_console/src/text_console_cmd.c`
- `main/main.c` (startup application of saved log level; any user-facing messages only if needed)
- App/component sources with `ESP_LOG*` adjustments per policy (examples): `components/bluetooth/src/bluetooth.c`, `components/wifi/src/wifi_cmd.c`, `components/websocket/src/websocket.c`, `components/http_server/src/http_server_cmd.c`, `components/filesystem/src/vfs.c`, `components/filesystem/src/flash.c` (expand as discovered)
- Docs/help text mentioning `logging` levels or serial/console terminology (as needed)

## Dependencies / prerequisites
- None beyond normal ESP-IDF/NVS/tooling for builds/tests; hardware optional depending on validation performed.

## Risks / constraints
- `debug` increases log volume/noise and can affect timing/perf on constrained hardware; default remains WARN.
- Global level (`esp_log_level_set("*", ...)`) affects all tags (per-tag control out of scope).
- Vendored `esp_littlefs` VERBOSE remains noisy and is intentionally out of scope here.

## Open questions
- None.

## Progress / evidence
- 2026-11-19: created from ponytail ultra logging review (session).
- 2026-11-19: clarified via `clarify-feature` flow (chat Q&A). Decisions: expose `debug` (no `verbose` now); default remains WARN on reset/invalid; apply logging policy by demoting/removing noisy app logs and documenting policy; terminology “UART/serial output and on-screen text console (mirrored stream)”; `esp_littlefs` VERBOSE out of scope with cross-link to `docs/plan/needs-clarification/esp-littlefs-stability-hardening.md`; approved “Go to Ready” by user.