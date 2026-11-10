/**
 * `ui` module registered by the Berry runtime.
 * Names and argument order match `berry_ui_register`.
 */

declare const labelBrand: unique symbol;
declare const buttonBrand: unique symbol;
declare const groupBoxBrand: unique symbol;
declare const checkboxBrand: unique symbol;

declare global {
  /**
   * Display, widgets, touch, and keyboard for the running Berry app.
   *
   * Widget positions are pixels, relative to the parent. Pass `0` to mount on
   * the screen, or a handle from {@link groupbox} to mount inside that group.
   * Keep the handle returned by each constructor; a plain `number` is not a
   * handle. Constructors return `null` when the app is not active or the
   * parent is not the screen or a group box.
   *
   * Colors are RGB565. Build them with {@link color565}.
   * Direct drawing skips the widget tree. Wrap a batch in {@link start_write}
   * and {@link end_write}.
   */
  namespace ui {
    /** RGB565 color, or a raw 16-bit value accepted by the display. */
    type Color = number;

    /** Handle of a label created by {@link label}. */
    type LabelHandle = number & { readonly [labelBrand]: "label" };
    /** Handle of a button created by {@link button}. */
    type ButtonHandle = number & { readonly [buttonBrand]: "button" };
    /** Handle of a group box created by {@link groupbox}. */
    type GroupBoxHandle = number & { readonly [groupBoxBrand]: "groupbox" };
    /** Handle of a checkbox created by {@link checkbox}. */
    type CheckboxHandle = number & { readonly [checkboxBrand]: "checkbox" };

    /** Any widget handle. `0` is the screen, not a widget. */
    type WidgetHandle = LabelHandle | ButtonHandle | GroupBoxHandle | CheckboxHandle;
    /** `0` is the screen. A group box mounts children inside itself. */
    type Parent = 0 | GroupBoxHandle;

    /** Touch position in screen pixels. */
    type Touch = [x: number, y: number];
    /** Widget bounds in screen pixels: x, y, width, height. */
    type Bounds = [x: number, y: number, width: number, height: number];

    /** Horizontal alignment constant exposed by the module. */
    const LEFT: 1;
    /** Horizontal alignment constant exposed by the module. */
    const CENTER: 0;
    /** Horizontal alignment constant exposed by the module. */
    const RIGHT: 2;

    /** USB HID usage: Enter. */
    const KEY_ENTER: 0x28;
    /** USB HID usage: Escape. */
    const KEY_ESCAPE: 0x29;
    /** USB HID usage: Space. */
    const KEY_SPACE: 0x2c;
    /** USB HID usage: A. */
    const KEY_A: 0x04;
    /** USB HID usage: D. */
    const KEY_D: 0x07;
    /** USB HID usage: Left arrow. */
    const KEY_LEFT: 0x50;
    /** USB HID usage: Right arrow. */
    const KEY_RIGHT: 0x4f;
    /** USB HID usage: Down arrow. */
    const KEY_DOWN: 0x51;
    /** USB HID usage: Up arrow. */
    const KEY_UP: 0x52;

    /** Default text color, black (`0x0000`). */
    const TEXT_COLOR: 0x0000;
    /** Default window background, white (`0xFFFF`). */
    const WINDOW_BG: 0xffff;
    /** Default button highlight, white (`0xFFFF`). */
    const BUTTON_HIGHLIGHT: 0xffff;
    /** Default button shadow, black (`0x0000`). */
    const BUTTON_SHADOW: 0x0000;

    /**
     * Add a label.
     *
     * @param parent `0` for the screen, or a group box.
     * @param text Initial text.
     * @param x Left edge, relative to `parent`.
     * @param y Top edge, relative to `parent`.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @returns A label handle, or `null` when the widget cannot be created.
     */
    function label(
      parent: Parent,
      text: string,
      x: number,
      y: number,
      width: number,
      height: number,
    ): LabelHandle | null;

    /**
     * Add a button.
     *
     * @param parent `0` for the screen, or a group box.
     * @param text Initial caption.
     * @returns A button handle, or `null` when the widget cannot be created.
     */
    function button(
      parent: Parent,
      text: string,
      x: number,
      y: number,
      width: number,
      height: number,
    ): ButtonHandle | null;

    /**
     * Add a group box. Its handle can be the parent of other widgets.
     *
     * @param parent `0` for the screen, or another group box.
     * @param title Title drawn on the frame.
     * @returns A group box handle, or `null` when the widget cannot be created.
     */
    function groupbox(
      parent: Parent,
      title: string,
      x: number,
      y: number,
      width: number,
      height: number,
    ): GroupBoxHandle | null;

    /**
     * Add a checkbox.
     *
     * @param parent `0` for the screen, or a group box.
     * @param text Caption beside the box.
     * @returns A checkbox handle, or `null` when the widget cannot be created.
     */
    function checkbox(
      parent: Parent,
      text: string,
      x: number,
      y: number,
      width: number,
      height: number,
    ): CheckboxHandle | null;

    /**
     * Replace the text of a label, button, or checkbox.
     * Group boxes keep the title they were created with.
     */
    function set_text(handle: LabelHandle | ButtonHandle | CheckboxHandle, text: string): void;

    /**
     * Recolor a label.
     *
     * @param background Defaults to white (`0xFFFF`) when omitted.
     */
    function set_text_color(handle: LabelHandle, foreground: Color, background?: Color): void;

    /** Move and resize a widget. The previous rectangle is cleared to white. */
    function set_bounds(handle: WidgetHandle, x: number, y: number, width: number, height: number): void;

    /**
     * Read a widget's current bounds, in screen pixels.
     *
     * @returns `[x, y, width, height]`, or `null` when the handle is not live.
     */
    function bounds(handle: WidgetHandle): Bounds | null;

    /** Call `callback` when the button is clicked. Replaces a previous handler. */
    function on_click(handle: ButtonHandle, callback: () => void): void;

    /**
     * Call `callback` when the checkbox changes.
     * The argument is the new checked state. Replaces a previous handler.
     */
    function on_change(handle: CheckboxHandle, callback: (checked: boolean) => void): void;

    /** Set whether a checkbox is checked. */
    function set_checked(handle: CheckboxHandle, checked: boolean): void;

    /**
     * Read a checkbox.
     *
     * @returns The checked state, or `false` when the handle is not a checkbox.
     */
    function get_checked(handle: CheckboxHandle): boolean;

    /**
     * Remove widgets.
     *
     * @param parent `0` removes every widget. A group box removes its descendants.
     */
    function clear(parent: 0 | GroupBoxHandle): void;

    /**
     * Pack 8-bit channels into an RGB565 color.
     * The display keeps the top 5 red bits, top 6 green bits, and top 5 blue bits.
     *
     * @param red Channel from 0 to 255.
     * @param green Channel from 0 to 255.
     * @param blue Channel from 0 to 255.
     */
    function color565(red: number, green: number, blue: number): Color;

    /** Ask the UI to redraw on the next frame. */
    function mark_dirty(): void;

    /**
     * Random integer in the range `0 .. limit - 1`.
     *
     * @returns `null` when `limit` is not positive.
     */
    function random(limit: number): number | null;

    /**
     * Ask COS to close this app.
     * `teardown` runs after the request is processed, not inside this call.
     */
    function exit(): void;

    /**
     * Explicitly launch a UI app (R21). Closes/replaces the active UI app,
     * constructs the app if a class is passed, calls `setup(0, width, height)`,
     * and starts the UI. Intended to be called from `main(args)` in a Berry
     * entrypoint (TS UI entrypoints lower to `return ui.launch(AppClass)`).
     *
     * @param app UI app class or instance.
     * @returns 0 on success. Raises on failure.
     */
    function launch(app: unknown): number;

    /** Fill the framebuffer with one color. The console renderer stays paused. */
    function fill_screen(color: Color): void;

    /**
     * Fill a rectangle in screen pixels.
     *
     * @param x Left edge.
     * @param y Top edge.
     * @param width Width in pixels.
     * @param height Height in pixels.
     */
    function fill_rect(x: number, y: number, width: number, height: number, color: Color): void;

    /**
     * Draw a single line of text in screen pixels.
     *
     * @param color Glyph color.
     * @param background Color of the text cell behind the glyphs.
     */
    function draw_text(x: number, y: number, text: string, color: Color, background: Color): void;

    /**
     * Current touch position.
     *
     * @returns Screen coordinates, or `null` when the screen is not touched.
     */
    function touch(): Touch | null;

    /**
     * Poll a Bluetooth HID key.
     *
     * @param keycode USB HID usage, for example {@link KEY_LEFT} or {@link KEY_A}.
     * Usages outside `1..127` are reported as up.
     * @returns `true` while that key is held.
     */
    function key_down(keycode: number): boolean;

    /** Begin one SPI write transaction for the following draw calls. */
    function start_write(): void;

    /** Finish the write transaction started by {@link start_write} and wait for it. */
    function end_write(): void;

    /**
     * Repeat `callback` until the app closes.
     *
     * @param interval_ms Delay between calls, in milliseconds. A non-positive
     * value does not start a timer.
     */
    function timer(interval_ms: number, callback: () => void): void;
  }
}

export {};
