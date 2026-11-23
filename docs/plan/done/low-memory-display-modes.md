# Low-memory display modes (v1: console char-mode, no row sprite)

- **ID:** low-memory-display-modes
- **Status:** done/delivered but **hardware validation BLOCKING/FAILED** (PR [#113](https://github.com/gallayl/cos/pull/113) draft; CI green; **user-reported on CYD: no console screen output**; requires investigation/fix before merge)
- **Created:** 2026-11-17
- **Updated:** 2026-11-23
- **Source/links:** User feature idea/discussion (2026-11-17): minimize display-buffer memory consumption by introducing alternative screen modes (character display, limited palette/colors, retro-style). Feasibility and RAM estimates discussed in-session; recommendation accepted: **focused v1 for biggest wins only**. PR: [#113](https://github.com/gallayl/cos/pull/113). Related context: [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (Display buffer memory / retro modes), [`../done/text-console-memory-lifecycle.md`](../done/text-console-memory-lifecycle.md), [`../../history/2026-10-improvements.md`](../../history/2026-10-improvements.md) (Milestone 4 scrollback storage note).
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to record decisions, mark **ready (approved)**, and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
Add a **low-memory console/text character-cell render mode (v1)** for CYD (ESP32 + ILI9341 240×320 RGB565, LovyanGFX) that removes the current 16bpp console row-sprite buffer and renders console rows/cells on-the-fly to the panel. Goal: deliver the **largest practical RAM win with contained risk** (no PSRAM). Broader retro/palette graphics modes are explicitly deferred.

**Current-state (code-reviewed):**
- Panel is RGB565-driven; no single full-screen software framebuffer found by default. Main console render buffer is `components/display/src/display.cpp` row sprite used by `display_draw_text_row()` (`lcd().width() * 8 * text_size * 2`, ~**3.75 KB** @ size 1, **7.50 KB** @ size 2, **11.25 KB** @ size 3), recreated on rotation/text-size change.
- Text console (`components/text_console/`) keeps a fixed visible grid and optional dynamic scrollback (`text_history_cell_t (*scrollback)[MAX_COLS]`, up to 128 rows, ~**13.5 KB** when allocated) rendered via the row-sprite path. Visible grid storage is significant (2160 cells; struct/padding-dependent, est. ~8.4–12.7 KB).
- Berry UI/native drawing may allocate additional sprites/buffers (not in v1 scope).

**Best-case RAM estimates (v1-relevant, no PSRAM):**
| Buffer/type | Current (est.) | v1 best-case | Save |
|---|---:|---:|---:|
| Console row sprite (16bpp) | 3.75–11.25 KB | ~0 (or tiny scratch ≪1 KB) | **3.75–11.25 KB (100%)** |
| Visible console cells (optional compacting) | ~8.4–12.7 KB | ~4.2 KB (2 B/cell) if included | **0–8.4 KB (0–67%)** |
| Scrollback (policy-dependent) | ~13.5 KB | unchanged in v1 default | **0 KB** (defer reduction/removal) |
| **Representative console subtotal** (size 1, scrollback on) | ~25.7–29.9 KB | ~17.7–26.2 KB | **~3.75–12 KB (~13–40%)** |

**Recommendation (adopted for v1 spec):** implement **console char-mode without row sprite** first (mandatory win). Visible-cell/dirty-storage compaction is **included in v1 only if low-risk and measured** (see Decisions). **Defer** scrollback-as-a-win, full 240×320 indexed/palette framebuffers (not viable in internal RAM), and broad Berry `ui` retro-graphics modes. Proceed only if on-device measurements show the gain matters for memory pressure.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Approval/go:** Approved v1 spec for implementation and moved to `docs/plan/ready/` (pre-approved for implementation/PR flow; merge remains human).
- **Visible-cell/dirty-storage compaction:** **Include in v1 if low-risk and measured.** Allow compaction in the same v1 scope only if changes remain low-risk and before/after struct/allocation sizes and behavior are verified on hardware (evidence recorded). Keep explicitly bounded; do not expand scope to scrollback/indexed/full-framebuffer/UI retro modes.
- **Mode selection surface:** **Compile-time/config only for v1.** No new runtime/script-facing control in v1; keep consistent with existing patterns. Toggle can be added later if needed (separate scope).
- **Compatibility bounds:** **Preserve current behavior; document only measured deviations.** Target behavior-identical console output/interaction for currently supported text/ANSI/color/scrollback/cursor/resize/rotation/font-size paths. Any deviation discovered during implementation/validation must be explicitly documented with evidence; no intentional regressions.
- **Scratch buffer policy:** **No extra persistent buffer; minimal bounded scratch only if required.** Prefer direct per-cell/per-glyph pushes with no extra persistent pixel buffer. Allow a minimal, fixed/bounded scratch buffer only if required for correctness/SPI efficiency/performance, with size/cap documented and lifecycle managed.

## Scope (v1)
- **In scope:**
  - Introduce a low-memory console/text render path that **does not allocate/use the 16bpp row sprite** for `display_draw_text_row`/console row rendering (direct glyph/cell expansion to RGB565 at push time, or a minimal fixed/bounded scratch buffer only if required per Decisions).
  - Keep logical console behavior compatible where feasible: character grid, cursor/ANSI handling, existing color/attribute semantics supported by current console, scrollback behavior, rotation/font-size support (as supported), and shell/text-console usage, preserving current behavior per Decisions.
  - Buffer/lifecycle: eliminate/release row-sprite allocation for the v1 path; no new large persistent pixel buffer introduced. Mode/path must have clear ownership and cleanup on init/deinit/resize/rotation/font-size changes as applicable.
  - **Visible-cell/dirty-storage compaction (included if low-risk and measured):** compact visible-cell representation/metadata (e.g. reduce per-cell bytes/padding or dirty tracking) without changing external behavior only when low-risk and verified on hardware; record before/after struct/allocation sizes and behavior evidence.
  - **Selection mechanism (v1):** compile-time/config only (no runtime/script-facing control in v1). Avoid broad SDK/API surface unless required (if script-facing API/SDK changes are introduced, keep `frontend/src/cos-sdk/` and `components/berry_lang/src/` in sync and add/compile device examples per repo rules).
  - On-device validation on CYD with before/after memory measurements and console smoke tests.
- **Out of scope (deferred follow-ups / separate items):**
  - Scrollback depth reduction/removal as a memory-saving policy (UX/behavior tradeoff; separate decision).
  - Full-screen indexed/palette graphics framebuffer modes (240×320 @ 4/8bpp too large for internal RAM) and generalized palette-graphics API.
  - Broad Berry `ui`/application retro modes, sprite-system redesign, or unrelated UI/app rewrites.
  - PSRAM-dependent designs.
  - Runtime/script-facing mode toggles beyond compile-time/config (unless separately scoped/approved).

## Non-goals
- Emulating specific vintage hardware timing/quirks beyond practical text-mode appearance/behavior.
- Hardware/panel protocol changes.
- Large unrelated refactors, new end-user apps, or optimizations without measured evidence.

## Acceptance criteria
- [ ] v1 console render path does **not** create/retain the 16bpp row sprite buffer for console rendering; any replacement scratch buffer is minimal, fixed/bounded, and documented (per scratch buffer policy).
- [ ] Measured before/after evidence on CYD shows row-sprite allocation removed and reports free heap, largest block, minimum watermark (and relevant static/alloc sizes) for representative scenarios (at least default text size with console/scrollback active); observed savings meet or are honestly reported against target (**~3.75–11.25 KB** from row sprite; higher only if compaction included with evidence).
- [ ] Console functional smoke on hardware: text output, cursor movement/ANSI sequences used today, colors/attributes, scrolling/scrollback behavior, resize/rotation/font-size behavior (as supported), and shell interaction remain correct; any measured deviation is explicitly documented with evidence (no intentional regressions).
- [ ] No new persistent large display buffer introduced; lifecycle/cleanup correct across init/deinit/resize/rotation/font-size paths touched. If compaction is included, it is low-risk, behavior-preserving, and before/after struct/allocation sizes are recorded.
- [ ] Automated checks for touched scope pass with recorded evidence (per `AGENTS.md`): at minimum `make format-check`, `make test`; plus `make build`/`make lint` as feasible for firmware changes (or blockers documented with actionable steps). `make docs-check` if docs/comments require it. Frontend/SDK/example checks only if script-facing surfaces change.
- [ ] Deferred items (scrollback policy, indexed graphics, Berry UI retro modes, runtime/script toggle) remain tracked as out-of-scope/follow-ups, not partially implemented.

## Validation plan
- **Automated (implementation PR/slice):** `make format-check`; `make test`. For firmware/C++ changes: `make build` and `make lint` in ESP-IDF v6.1 environment when feasible (report exact results/blockers). `make docs-check` when applicable. If `frontend/`, `storage/ts`, `storage/flash_data`, or SDK/native script bindings change: `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` in `frontend/` and keep compiled device examples in sync.
- **Hardware/manual (required):** on ESP32-2432S028R (CYD), capture baseline vs v1: row-sprite buffer presence/size, heap/free/largest block/minimum watermark at boot/console-ready/after scrollback allocation/after representative output; verify console correctness (output, ANSI/color, scroll, scrollback, clear/resize/rotation/font-size if in scope) and note render latency/CPU observations. Record exact firmware/build, commands, and measurements in progress/evidence or linked report. If compaction included, record before/after struct/allocation sizes and behavior evidence.

## Expected areas/files
- `components/display/include/display.h`, `components/display/src/display.cpp` — remove/bypass row-sprite dependency for console path; add minimal glyph/row push primitive or bounded scratch as needed.
- `components/text_console/src/text_console.cpp`, `components/text_console/src/text_buffer.*`, `components/text_console/include/text_console.h` — render path adaptation, optional/conditional compact cell/layout changes, lifecycle/resize/rotation handling.
- `components/shell/`, `main/`, config/build options — only if compile-time/config selection is added.
- `components/berry_lang/src/` and `frontend/src/cos-sdk/` — **only if** script-facing API is introduced/changed (must stay in sync) plus `storage/ts`/`storage/flash_data` examples if applicable.
- `docs/` — progress/evidence updates and follow-up split items if needed.

## Dependencies / prerequisites
- Explicit approval recorded (2026-11-20) and item promoted to `plan/ready/` (this document).
- Baseline on-device measurement method/environment (ESP-IDF v6.1, CYD hardware access).
- Coordination with existing console/lifecycle semantics (`text-console-memory-lifecycle`) to avoid regressions.

## Risks / constraints
- **Net gain may be modest for workload:** ~4–11 KB row-sprite win is meaningful in pressure cases but not transformative system-wide; must be justified by measurements.
- **CPU/SPI/latency tradeoff:** on-the-fly glyph expansion can increase render cost; must observe on hardware.
- **Compatibility/regression risk:** ANSI/colors, scrollback, rotation, font scaling, and partial-row updates may differ subtly without a row sprite.
- **Optional compaction risk:** changing cell struct/dirty tracking can introduce bugs; include only if low-risk and measured with evidence.
- **No PSRAM:** full indexed framebuffers remain out of reach; do not expand scope to them.

## Open questions
- None (user decisions recorded 2026-11-20, clarify-feature: approval/go, compaction in v1 if low-risk and measured, compile-time/config selection only, preserve current behavior with only measured deviations documented, no extra persistent buffer with minimal bounded scratch only if required).

## Progress / evidence
- 2026-11-17: created (needs-clarification) with initial feasibility review.
- 2026-11-17: spec finalized with recommendation (v1 = console char-mode without row sprite; optional compaction gated; indexed/full-framebuffer and broad UI modes deferred); best-case RAM estimates recorded.
- 2026-11-20 (clarify-feature, user approval): recorded decisions (approve/move to ready; compaction in v1 if low-risk and measured; compile-time/config selection only; preserve current behavior with only measured deviations documented; no extra persistent buffer, minimal bounded scratch only if required). Open questions cleared; status set to **ready (approved)** and moved from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
- 2026-11-23 (implementation, branch `feat/low-memory-display-console-char-mode` from `origin/main@d7f0c0b`, baseline green per main CI evidence): implemented v1 low-memory console row render path. Added `components/display/Kconfig` option `CONFIG_COS_DISPLAY_LOW_MEMORY_CONSOLE` (default `y`, compile-time/config only). When enabled, `display_draw_text_row()` renders directly to LCD (`startWrite` + full-row `fillRect` + per-char `setCursor/setTextColor/write` + `endWrite`) with no row-sprite allocation/retention; legacy row-sprite path retained under `#else` for compile-time fallback. Removed row-sprite rebuild on rotation/text-size in low-memory path. Updated `display.h` Doxygen to reflect config-neutral behavior. **Visible-cell/dirty-storage compaction not included** (deferred as not low-risk/measured in this slice; row-sprite removal is the primary win). **Validation (exact, local):** `make format-check` — **PASS** (exit 0); `make test` — **PASS** (exit 0, 31/31 CTest, 3/3 Python unittest); `make docs-check` — **PASS** (exit 0, doxygen, zero warnings reported by target); `make build` — **PASS** (exit 0, ESP-IDF v6.1, `Project build complete`, `cos.bin` 0x1cdde0). `make lint` — **FAIL** (exit 1) due to pre-existing/unrelated clang-tidy/clang-diagnostic issues in other components (e.g. `berry_http.cpp`, missing headers/compile-db resolution in unrelated files); `Select-String` on lint output for `display.cpp|display.h` returned **no matches**. **Not run / remaining (hardware required, blocking for merge-ready evidence):** on-device CYD before/after heap/watermark/largest-block/row-sprite allocation measurements, console smoke (text/ANSI/color/scrollback/cursor/resize/rotation/font-size/shell interaction), and render-latency/CPU observation per acceptance criteria. No script-facing SDK/Berry API changes.
- 2026-11-23 (independent review fix, session `ses_df87aa926ffe6X8Nnlh3YguoXR`): addressed finding 1 by saving/restoring LCD font and text size around the low-memory row draw and removing `setTextWrap(false)` (avoids global LCD text-state leak; no wrap getter in pinned LovyanGFX). Addressed finding 3 by narrowing `display_draw_text_row` Doxygen return contract to "invalid input or row rendering could not be started". Finding 2 (no dedicated automated host coverage for default low-memory branch; host tests mock display and Kconfig macro is absent in host builds) remains a known gap: CI integration/QEMU boots exercise console rendering indirectly under default config, but no dedicated assertion targets this path; required CYD hardware validation remains blocking. Re-validation after fixes: `make format-check` **PASS**, `make test` **PASS** (31/31 CTest, 3/3 Python), `make docs-check` **PASS**, `make build` **PASS** (ESP-IDF v6.1, exit 0). `make lint` not re-run (prior failure was pre-existing/unrelated; no display-file findings).
- 2026-11-23 (PR/CI/babysit): PR [#113](https://github.com/gallayl/cos/pull/113) opened (draft) from `feat/low-memory-display-console-char-mode` to `main`. PR CI run [45140445465](https://github.com/gallayl/cos/actions/runs/45140445465) (head `3a20c894`) **all required checks green**: Format Check/Required, frontend Required (frontend build/test skipped as non-applicable), Unit Tests/Required (31/31 CTest, 3/3 Python in CI), all integration suites pass (`esp_littlefs_core`, `esp_littlefs_extra`, `init_sequence`, `nvs_calibration`, `shell_fs`, `stability`, `system`, `vfs_ops`) + integration Required, lint/ESP-IDF Build & Lint + lint Required, CI Checks Required. Merge-blocking hardware validation comment posted: https://github.com/gallayl/cos/pull/113#issuecomment-7095712642. Item moved to `docs/plan/done/` per delivery criteria; **pending human review/approval, merge, and required on-device CYD hardware validation** (merge-blocking until evidence or explicit human waiver).
- 2026-11-23 (automated on-device console validation, partial; COM3): ran `scripts/hw_console_session.py` (pyserial 3.5) against COM3/115200 with reset. Boot log reported `cos` app `3a20c89-dirty`, ESP-IDF v6.1, ESP32 rev v3.1, `Display initialized (240x320)`. Artifacts persisted: [`docs/reports/artifacts/hw-2026-11-23-low-memory-display-com3/`](../../reports/artifacts/hw-2026-11-23-low-memory-display-com3/) (`serial-session.log`, smoke2-4 logs, `summary.md`, command files). Console-captured PASS: boot/prompt, `svc_mem` snapshots (early boot: default_free 59876/min 59876/largest 59392; after smoke: default_free 21364/min 7268/largest 8192), `services`, `help`, `display_mode text`, `clear` command acceptance, `screen brightness/rotation` (`128 manual`, `3`), `berry eval` (`SMOKE-OK`), scroll loop `scroll-0..35`, no panic/reset in logs. Inconclusive/unrelated: `ls /flash` failed due pre-existing `/sdcard/scripts/ls.be` `charAt` error. Not verified by agent: on-screen visual correctness, ANSI/cursor/color rendering, touch/latency feel, before/after row-sprite memory comparison (`svc_mem` does not expose text_console metrics). Evidence comment: https://github.com/gallayl/cos/pull/113#issuecomment-7096662321.
- 2026-11-23 (user hardware report, BLOCKING): user reported **no console screen output** on CYD during display validation. PR converted to draft; merge remains blocked pending investigation/fix and re-validation on hardware. No fix applied in this session.
