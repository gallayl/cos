# UI component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/ui/` (`include/ui.h`, `src/ui.cpp`, `CMakeLists.txt`), plus host tests in `test/test_ui.cpp` and observed usage in `components/berry_lang/src/berry_ui.cpp`, `components/berry_lang/src/berry_runtime.cpp`.  
**Load assumed:** One person using the device UI at a time (single touch stream) on ESP32-S3. UI draws happen from a dedicated FreeRTOS UI task; touch is polled from that task via `display_get_touch`. Berry/UI code may call `start/stop/set_*_callback/preserve_display_on_start` from other tasks or setup paths.

## What this repo does

`components/ui` is a small retained UI layer for the ESP32 display: a global root `Container`, basic widgets (`Label`, `Button`, `Checkbox`, `GroupBox`), touch dispatch, and a FreeRTOS task (`ui` task, 4 KB stack, priority 5 on ESP) that redraws when invalidated, polls touch, runs a tick callback, and can request app exit. Drawing is routed through `Renderer` / `DisplayRenderer` into the shared `display` component. Actions from widget callbacks are queued and run after touch release in the UI task path. There is a host-side Unity test suite (`test/test_ui.cpp`) that covers bounds, mounting, touch flow, and drawing calls via a fake renderer.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| U1 | Must fix | `start()`/`stop()` race the UI task (unsafe concurrent state changes) | Open |  | Touch redrawn from task while start/stop mutate globals/root without guard |
| U2 | Must fix | GroupBox draws title above its bounds (`y - 4`) | Fixed |  | 2026-11-13: `components/ui/src/ui.cpp` now draws GroupBox title at `bounds_.y + 1` (inside bounds) instead of `bounds_.y - 4`. |
| U3 | Must fix | Text rendering ignores bounds (overflow/clipping not enforced) | Open |  | `Renderer::draw_text` passes bounds to `display_draw_text` only as x,y; long text can overflow |
| U4 | Should fix | Touch end events forwarded to every child container path can misbehave with nested containers | Open |  | `Container::on_touch_end` calls all children; press state is per-widget but routing assumes different semantics |
| U5 | Should fix | `Container::clear()` unmounts children even if container not mounted (asymmetric) | Open |  | Unmount called on children inside `if (mounted_)` branch but clear is used broadly |
| U6 | Should fix | `TouchDispatcher::update` reuses `last_x/last_y` on release in odd cases | Open |  | Passing last coords on release can be confusing; release is outside widget in practice for cancel |
| U7 | Should fix | Checkbox uses hardcoded visual assumptions (box size 12, text offset) and may draw poorly for small bounds | Open |  | No validation; can look wrong for tiny heights |
| U8 | Nice to have | Button text color and background behavior not configurable (API inconsistency vs Label/Checkbox) | Open |  | Hardcoded white text on `background_` |
| U9 | Nice to have | `action_queue` move-then-clear is redundant; no cap on queued actions | Open |  | Minor cleanup / risk of unbounded growth if queued faster than drained |
| U10 | Nice to have | `start()` prints heap stats unconditionally on ESP even if task already exists | Open |  | Logs when re-entering active path; could be noisy |

**Status values:** Open → In progress → Fixed → Verified.

## Findings

1. **Must fix: start/stop can race the UI task and corrupt UI state** (`components/ui/src/ui.cpp:154-209`, `components/ui/src/ui.cpp:44-84`)
   - What this is: `start()` sets root bounds, mounts root, flips flags, and on ESP may create/resume the UI task. `stop()` sets `ui_active` false, clears root (`root.clear()` unmounts children, clears vector), resets `touch_dispatcher`, clears `action_queue` — all from the caller’s thread/context.
   - Problem: The UI task loops continuously (when inactive it still sleeps). It can be drawing (`root.draw`) or polling/touching `root` at the same time `stop()` clears or mutates the tree, or at the same time `start()` remounts/resizes. There is no execution guard used by `start/stop`, and no mutex around root/touch_dispatcher/action_queue. This is a data race/use-after-free risk on ESP.
   - Concrete case: Call `ui::stop()` while the UI task is inside `root.draw(display_renderer)`. `stop()` calls `root.clear()` which destroys children and unmounts them; the task may still dereference those elements → crash or corrupted draw. Same risk when Berry/runtime code restarts UI.
   - Fix: Serialize all UI tree mutations and redraw entrypoints with the same guard used by the task (`set_execution_guard`) or a dedicated mutex. Make `stop()` wait/join behavior safe (or require UI inactive and single-threaded) and document it. Alternatively, only allow `start/stop` from the UI task context. Smallest safe step: take execution lock in `start/stop` if provided, and ensure task exits cleanly before teardown.
   - If we skip it: Crashes or random drawing glitches when restarting UI or when other code touches UI while running.

2. **Must fix: GroupBox draws its title above the widget bounds** (`components/ui/src/ui.cpp:372-402`, `components/ui/include/ui.h:87-101`)
   - What this is: `GroupBox::draw` computes title position as `bounds_.y - 4` and draws text there, while the box border/top line is drawn at `bounds_.y`.
   - Problem: The title is rendered outside the group box’s own bounds (above it). If the group is placed near the top of the screen, this can draw into negative/invalid regions or over other widgets placed above; clipping is not done by UI. This is a visible layout bug.
   - Concrete case: A `GroupBox` at `y == 2` with default behavior draws title at `y == -2` via `display_draw_text` (no bounds clipping in UI) — text may wrap/cut oddly or write off-screen depending on driver.
   - Fix: Draw the title inside the box (e.g. below top border or inside the top area) and adjust layout math so title stays within the widget’s drawable area, or require caller to reserve space. Keep change minimal: move title down into the box and shift children area accordingly.
   - If we skip it: Titles can appear clipped, misplaced, or overwrite other UI content near the top.

3. **Must fix: Text is not clipped to the widget bounds** (`components/ui/src/ui.cpp:102-106, 398-401, 433-435, 493-495, 562-563`, `components/display/include/display.h:56`)
   - What this is: `Renderer::draw_text` forwards `(bounds.x,bounds.y,text,fg,bg)` to `display_draw_text`; widgets pass `bounds_` (full element bounds) for Label/Button/Checkbox text areas.
   - Problem: No width/height clipping is done by the UI layer. Long text will be drawn past the right/bottom edge of the intended area (driver behavior varies). This can overwrite other widgets or look broken.
   - Concrete case: A `Label` with width 40 pixels showing a long string draws past its bounds into neighboring space; a `Button` label overflows the button.
   - Fix: Clip text drawing to the widget’s bounds (at minimum by not drawing past `bounds.x + bounds.width`). The smallest practical fix is to truncate/pad logic isn’t desired, but you can limit by character count based on font width assumptions only if consistent; better: document and/or implement a simple clip (e.g. only draw if starting inside, and rely on driver — but not guaranteed). Real fix needs font metrics; if unavailable, at least avoid passing oversized intent and note limitation. Prefer the smallest change: add a comment + truncate string to fit by an estimated char width only where cheap, or enforce in callers — but current API gives no font info. This is a design gap.
   - If we skip it: Text spills over UI elements and makes screens unreadable or causes visual corruption.

4. **Should fix: Container forwards touch end to every child** (`components/ui/src/ui.cpp:340-352`)
   - What this is: `Container::on_touch_end` iterates all children and calls `on_touch_end(x,y)` on each, while `on_touch` routes to the topmost child under the point.
   - Problem: For nested containers, a child that received `on_touch` might not be the same set considered on end in all cases; also widgets track `pressed_` by `contains()` at end time. This can cause odd behavior if a press started on a child that later moved, or in complex nests. Current flat usage works by coincidence for simple cases.
   - Concrete case: A press starts inside a button inside a container; if layout changes before release (unlikely here) or with nested hit areas, end may be delivered to wrong widgets. Also all children get end calls even if not pressed — cheap but semantically loose.
   - Fix: Track which child received the press (store pointer/id) and only dispatch end to it, falling back to hit-test if needed. Smallest fix: remember `pressed_child_` in `TouchDispatcher` or in `Container` for the active press.
   - If we skip it: Rare misfires on complex UI trees; current simple apps are mostly fine.

5. **Should fix: clear() unmounts children only when container is mounted** (`components/ui/src/ui.cpp:270-282`)
   - What this is: `clear()` calls `child->unmount()` for each child only if `mounted_` is true, then clears the list and invalidates.
   - Problem: If children were mounted while container was mounted, that’s fine. But if someone adds children and clears before mount, children never got mounted so unmount is a no-op — okay. However, the asymmetry is confusing; also `clear()` forces a redraw with clear background even if nothing drawn. Not a crash but odd state handling.
   - Concrete case: Normal use is fine; edge cases around mount order can leave `mounted_` flags inconsistent if external code toggles state oddly.
   - Fix: Always call `unmount()` on removed children (idempotent) to keep state consistent: `for (auto &c: children_) c->unmount();` regardless of `mounted_`.
   - If we skip it: Minor confusion and possible stale `mounted_` flags on removed widgets.

6. **Should fix: TouchDispatcher passes stale coords on release** (`components/ui/src/ui.cpp:582-609`)
   - What this is: On release (`!is_down && was_down_`), it calls `root.on_touch_end(x, y)` using `last_x_/last_y_` if the driver didn’t provide a new point (common). But if touch ended outside, using last known point means end is delivered at last point — widgets cancel press if not contains(last). That can be wrong if release happened far away; behavior depends on intent.
   - Problem: The API treats release position as the point for end; using last polled point is a heuristic. Can cause a button to not cancel properly in some cases.
   - Concrete case: User presses inside button, drags finger off quickly and lifts; last sample may be inside → button could incorrectly register click. (Current test does move outside then release; driver gives false + coords? code sets last when down, on release uses last_x/last_y passed in — in poll they pass last when not down: `update(root, is_down, is_down?x:last_x, is_down?y:last_y)`. So release uses last touch point.)
   - Fix: On release, prefer the current reported point if valid; otherwise treat as cancel (no click) by dispatching end at last point but widgets already cancel if released outside — current behavior matches “released outside cancels”, which is fine. Document it; no change needed if intended.
   - If we skip it: Occasional wrong-click feel in fast drags (low risk with current simple widgets).

7. **Should fix: Checkbox draw/layout assumptions are fragile** (`components/ui/src/ui.cpp:470-516`)
   - What this is: Checkbox draws a 12px box, hardcoded offsets, draws a short check mark if `box_size>=6`.
   - Problem: For small bounds (e.g. height < 12) box is clamped to height but layout can look odd; text may overlap box area depending on bounds. No guard for very small widths.
   - Concrete case: `Checkbox` with height 10 and width 20 will draw a 10x10 box plus text starting at x+14 into only 6px width — text likely clipped/overwritten badly.
   - Fix: Clamp text area and skip drawing text if no space; optionally enforce minimum bounds in docs or code. Small change: `if (text_width <= 0) { /* skip text */ }` is there, good — but visual overlap if text_x is inside? No, text starts at text_x. Still fine if clipped by driver, but better to be safe.
   - If we skip it: Ugly rendering on tiny widgets (rare if laid out carefully).

8. **Nice to have: Button has no configurable colors (unlike Label/Checkbox/GroupBox)** (`components/ui/include/ui.h:141-161`, `components/ui/src/ui.cpp:545-581`)
   - What this is: Button only has text and callback; colors are hardcoded (background default 0xC618, text white, frame colors fixed).
   - Problem: API inconsistency forces all buttons to look the same; theming is harder.
   - Fix: Add `set_colors(...)` like other widgets. Low urgency.
   - If we skip it: Slightly less flexible UI theming.

9. **Nice to have: action queue can grow unbounded and move+clear is odd** (`components/ui/src/ui.cpp:33-38, 107-124`)
   - What this is: `action_queue()` is a static vector; `execute_queued_actions()` moves it then clears (redundant). No limit.
   - Problem: If callbacks queue many actions and they aren’t drained in some path, memory can grow (embedded RAM). Also minor code smell.
   - Fix: Replace with `std::queue` or just swap into a local vector and `clear()` (no move-then-clear). Consider a max size guard/log.
   - If we skip it: Unlikely in practice; minor cleanup opportunity.

10. **Nice to have: start() logs heap info even when task already running** (`components/ui/src/ui.cpp:161-184`)
   - What this is: When `ui_task_handle != nullptr`, `start()` sets `ui_active=true` and returns true without logging; but the heap log only happens on first create. Not directly an issue. (Minor: message says “Starting UI task…” only on create — accurate.)
   - Problem: None real; just noting style. No action needed.
   - Fix: N/A.
   - If we skip it: Nothing.

## Verdict

The UI module is small and has decent host tests for core behavior, but it has real safety problems on ESP: `start()`/`stop()` are not synchronized with the UI task, so restarting or tearing down UI while it runs can crash or corrupt memory. Layout bugs (GroupBox title drawn above bounds, no text clipping) will cause visible glitches. Fix U1–U3 before relying on dynamic start/stop or complex screens.

## Lean

- Lean: no line/dependency removals identified from this audit (module is already small). Possible small cleanup: remove redundant move+clear in action queue, simplify `clear()` unmount logic (~few lines). No new dependencies suggested.

## Validation / not checked

- 2026-11-13: Ran `make format-check` and `make test` after U2 fix — **pass** (host checks, including `test_ui`). Did not run ESP-IDF firmware build/lint, QEMU, or on-device UI validation.
- Not checked: full ESP-IDF build/on-device behavior, `display_*` clipping details, Berry integration end-to-end, performance under load.