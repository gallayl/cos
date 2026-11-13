# Display component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/display/` (`include/display.h`, `src/display.cpp`, `src/cyd_board_config.h`, `CMakeLists.txt`, `idf_component.yml`), plus observed usage in `main/main.c`, `components/ui/src/ui.cpp`, and `components/text_console/src/text_console.cpp`.  
**Load assumed:** One person using one CYD device (ESP32-2432S028R, ILI9341 240x320 + XPT2046 touch). Display and touch are used by a single UI flow at a time; UI drawing is batched in a UI task, and the text console draws rows when it owns the screen (console rendering is paused while a Berry UI app is active per README). No multi-user or multi-process load.

## What this repo does

`components/display` is a small C wrapper around LovyanGFX for the CYD board. It sets up the SPI panel, PWM backlight, and touch controller, and exposes init, rotation, brightness, touch read, calibration, and basic drawing functions (clear/fill, lines, text, character, and a batched text-row sprite). `main` initializes it at boot, `ui` draws widgets through it, and `text_console` pushes full rows through `display_draw_text_row`.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| D1 | Must fix | Row sprite is not rebuilt when rotation/display width changes | Fixed |  | 2026-11-13: `display_set_rotation` now deletes/rebuild-invalidates the row sprite so it is recreated at the new width. |
| D2 | Must fix | Row sprite creation result is not checked | Fixed |  | 2026-11-13: `display_draw_text_row` checks `createSprite` result, logs error, and returns without drawing on failure. |
| D3 | Should fix | `display_draw_text_row` does not validate inputs | Fixed |  | 2026-11-13: `display_draw_text_row` now returns early on null `chars`/`fg` or non-positive `count`. |
| D4 | Should fix | `display_draw_text` ignores null text and resets text size/font state | Open |  | Null can crash; size forced to 1 can surprise callers and shared state |
| D5 | Should fix | Display API does not guard use before initialization | Open |  | Most functions call into LovyanGFX without checking `display_init` ran |
| D6 | Nice to have | No tests for the display wrapper | Open |  | Rotation/sprite sizing and argument handling are untested |
| D7 | Nice to have | Module has no synchronization for shared mutable display state | Open |  | Safe only when callers serialize access; worth documenting or guarding if use grows |

**Status values:** Open → In progress → Fixed → Verified.

## Findings

### Must fix

1. **Row sprite keeps the old width after rotation changes** (`components/display/src/display.cpp:124-132,299-320`)
   - **What this is:** `display_draw_text_row` creates one row sprite (`row_sprite_ready`) sized to `lcd().width()` and reuses it. `display_set_rotation` only calls `lcd().setRotation(rotation)`.
   - **Problem:** After rotation, the screen width can change, but the sprite is not deleted or resized. The next row draw uses the old sprite width.
   - **Fix:** When rotation changes, invalidate the row sprite like `display_set_text_size` does: if `row_sprite_ready`, `row_sprite().deleteSprite()` and set `row_sprite_ready = false`. Keep this inside `display_set_rotation`.
   - **If we skip it:** Any future or runtime rotation change can draw console rows with the wrong width, leaving cropped or misaligned text until text size is changed or the device restarts.

2. **Row sprite allocation failure is treated as success** (`components/display/src/display.cpp:299-320`)
   - **What this is:** `display_draw_text_row` calls `row_sprite().createSprite(lcd().width(), font_h)` and then sets `row_sprite_ready = true` without checking whether the sprite was created.
   - **Problem:** On low memory, sprite creation can fail. The code still fills, writes into, and pushes from the sprite.
   - **Fix:** Check the create result and only mark ready on success. On failure, leave `row_sprite_ready` false, log an error once, and return without drawing.
   - **If we skip it:** A low-memory draw can corrupt memory or crash the device when rendering console rows.

### Should fix

3. **`display_draw_text_row` trusts pointers and count without checks** (`components/display/src/display.cpp:299-320`, `components/display/include/display.h:100-113`)
   - **What this is:** The function loops from `0` to `count - 1` and reads `chars[i]` and `fg[i]`.
   - **Problem:** If `chars` or `fg` is null, or `count` is negative/too large for the provided buffers, it reads invalid memory.
   - **Fix:** Return early when `!row_sprite_ready` setup cannot run safely, when `count <= 0`, or when `chars == nullptr || fg == nullptr`. Keep existing behavior for valid callers.
   - **If we skip it:** A bad caller can cause a crash or garbled row output; this is easy to hit during refactors because the API takes raw arrays.

4. **`display_draw_text` can crash on null and changes shared text state** (`components/display/src/display.cpp:223-237`, `components/display/include/display.h:55-67`)
   - **What this is:** `display_draw_text` passes `text` directly to `lcd().print(text)` and forces `setTextSize(1)`. `display_draw_char` also forces `setTextSize(1)`.
   - **Problem:** A null `text` pointer can crash inside the print call. Forcing text size to 1 also changes LovyanGFX text state, so later draws may not match the size set by `display_set_text_size` if callers mix APIs.
   - **Fix:** Treat null text as an empty/no-op draw. Avoid forcing size to 1 (or restore the previous size) so single-text draws do not silently override the configured text size.
   - **If we skip it:** A null string can crash the UI/console path, and mixed text APIs can render at the wrong size in hard-to-see ways.

5. **Most display functions do not check that init completed** (`components/display/src/display.cpp:98-325`)
   - **What this is:** `display_init` sets `initialized = true`, but other functions (`display_get_width`, drawing, touch, brightness, rotation) do not check it.
   - **Problem:** If a caller uses the API before init (or after a future failed init path), behavior depends on LovyanGFX/static object state and is not clearly guarded by this module.
   - **Fix:** Add a small guard for state that requires init (at least drawing/touch/read paths), returning a safe default or `ESP_ERR_INVALID_STATE` where the signature allows. Keep `display_init` as the single place that performs hardware setup.
   - **If we skip it:** Early or misordered calls can behave unpredictably and are harder to debug than an explicit init error.

### Nice to have

6. **The display wrapper has no automated tests** (`components/display/`, `test/`)
   - **What this is:** I found no host or Unity tests covering `components/display`; repo tests cover other modules and UI uses a fake renderer.
   - **Problem:** Changes to rotation, sprite sizing, text size, and argument handling have no fast safety net.
   - **Fix:** Add small host-side tests with a LovyanGFX/display fake or seam for `display_set_rotation` + row sprite invalidation and for null/invalid input guards.
   - **If we skip it:** Regressions in row rendering or basic guards are likely to be caught only on hardware.

7. **Shared display state is not synchronized inside the module** (`components/display/src/display.cpp:86-101,289-320`)
   - **What this is:** The module keeps shared globals (`initialized`, `current_brightness`, `s_text_size`, `row_sprite_ready`) and a shared static `lcd()`/sprite without its own lock.
   - **Problem:** This is fine when callers serialize access, but nothing in the API prevents concurrent calls from different tasks if usage expands.
   - **Fix:** Document the threading rule in the header (single-owner/serialized access), or add a lightweight lock around state-changing and draw paths if cross-task drawing becomes expected.
   - **If we skip it:** Today it is likely safe under current usage, but future cross-task callers could race on sprite state or SPI transactions.

## Verdict

Not healthy for future changes: fix row sprite invalidation and allocation checks first (D1–D2). D3–D5 are small hardening fixes that reduce crash risk.

## Lean: -0 lines, -0 dependencies possible.

No dead code or removable dependencies were confirmed in this module from this audit.

## Validation / not checked

- 2026-11-13: Ran `make format-check` and `make test` after D1–D3 fixes — **pass** (host checks). `components/display` wrapper behavior is not covered by host unit tests.
- Not checked: on-device behavior, SPI timing, touch accuracy, rotation changes at runtime, sprite allocation failure paths, LovyanGFX internals, `display_set_text_font` interactions, and ESP-IDF firmware build/lint/QEMU validation for this change.
