/**
 * Pragmatic R11 binding inventory: expected COS runtime surface that the
 * TypeScript SDK must cover. Derived from firmware bindings in
 * `components/berry_lang/src/berry_ui.cpp`, `berry_network.cpp`, and
 * `berry_runtime.cpp` (console/fs/globals). Update this when native bindings
 * change, then run the R11 audit tests.
 */

export type BindingKind = 'function' | 'const'

export type Binding = {
  kind: BindingKind
  name: string
  /** Internal/runtime hook exposed by Berry but not intended for app scripts. */
  internal?: boolean
}

export type NativeModuleInventory = {
  /** Berry/native module name (e.g. `ui`, `net`). */
  module: string
  /** C++ source file relative to repo root, used by cross-check tests. */
  source: string
  /** Global namespace declaration expected in SDK (e.g. `namespace ui`). */
  globalNamespace?: string
  /** `@cos/*`/`@berry/*` module specifiers that resolve to this module. */
  specifiers: readonly string[]
  bindings: readonly Binding[]
}

export type GlobalFunctionInventory = {
  name: string
  source: string
  /** Text expected in global SDK declarations (best-effort name check). */
  declarationPattern: string
}

export const cosNativeModules: readonly NativeModuleInventory[] = [
  {
    module: 'ui',
    source: 'components/berry_lang/src/berry_ui.cpp',
    globalNamespace: 'ui',
    specifiers: ['@cos/ui', '@berry/ui'],
    bindings: [
      { kind: 'function', name: 'init', internal: true },
      { kind: 'function', name: 'label' },
      { kind: 'function', name: 'button' },
      { kind: 'function', name: 'groupbox' },
      { kind: 'function', name: 'checkbox' },
      { kind: 'function', name: 'set_text' },
      { kind: 'function', name: 'set_text_color' },
      { kind: 'function', name: 'set_bounds' },
      { kind: 'function', name: 'bounds' },
      { kind: 'function', name: 'on_click' },
      { kind: 'function', name: 'on_change' },
      { kind: 'function', name: 'set_checked' },
      { kind: 'function', name: 'get_checked' },
      { kind: 'function', name: 'clear' },
      { kind: 'function', name: 'color565' },
      { kind: 'function', name: 'mark_dirty' },
      { kind: 'function', name: 'random' },
      { kind: 'function', name: 'exit' },
      { kind: 'function', name: 'fill_screen' },
      { kind: 'function', name: 'fill_rect' },
      { kind: 'function', name: 'draw_text' },
      { kind: 'function', name: 'touch' },
      { kind: 'function', name: 'key_down' },
      { kind: 'function', name: 'start_write' },
      { kind: 'function', name: 'end_write' },
      { kind: 'function', name: 'timer' },
      { kind: 'const', name: 'LEFT' },
      { kind: 'const', name: 'CENTER' },
      { kind: 'const', name: 'RIGHT' },
      { kind: 'const', name: 'KEY_ENTER' },
      { kind: 'const', name: 'KEY_ESCAPE' },
      { kind: 'const', name: 'KEY_SPACE' },
      { kind: 'const', name: 'KEY_A' },
      { kind: 'const', name: 'KEY_D' },
      { kind: 'const', name: 'KEY_LEFT' },
      { kind: 'const', name: 'KEY_RIGHT' },
      { kind: 'const', name: 'KEY_DOWN' },
      { kind: 'const', name: 'KEY_UP' },
      { kind: 'const', name: 'TEXT_COLOR' },
      { kind: 'const', name: 'WINDOW_BG' },
      { kind: 'const', name: 'BUTTON_HIGHLIGHT' },
      { kind: 'const', name: 'BUTTON_SHADOW' },
    ],
  },
  {
    module: 'console',
    source: 'components/berry_lang/src/berry_runtime.cpp',
    globalNamespace: 'console',
    specifiers: ['@cos/console', '@berry/console'],
    bindings: [
      { kind: 'function', name: 'read_line' },
      { kind: 'function', name: 'cancel_input' },
      { kind: 'function', name: 'read_key' },
      { kind: 'function', name: 'read_char' },
      { kind: 'function', name: 'move' },
      { kind: 'function', name: 'cursor' },
      { kind: 'function', name: 'size' },
      { kind: 'function', name: 'write' },
      { kind: 'function', name: 'write_at' },
      { kind: 'function', name: 'clear' },
      { kind: 'function', name: 'set_font_size' },
      { kind: 'function', name: 'font_size' },
    ],
  },
  {
    module: 'fs',
    source: 'components/berry_lang/src/berry_runtime.cpp',
    globalNamespace: 'fs',
    specifiers: ['@cos/fs', '@cos/filesystem', '@berry/fs', '@berry/filesystem'],
    bindings: [
      { kind: 'function', name: 'list' },
      { kind: 'function', name: 'read_text' },
      { kind: 'function', name: 'write_text' },
      { kind: 'function', name: 'read_json' },
      { kind: 'function', name: 'write_json' },
      { kind: 'function', name: 'exists' },
      { kind: 'function', name: 'is_dir' },
      { kind: 'function', name: 'is_file' },
      { kind: 'function', name: 'size' },
    ],
  },
  {
    module: 'net',
    source: 'components/berry_lang/src/berry_network.cpp',
    globalNamespace: 'net',
    specifiers: ['@cos/net', '@cos/network', '@berry/net', '@berry/network'],
    bindings: [
      { kind: 'function', name: 'ping' },
      { kind: 'function', name: 'host' },
      { kind: 'function', name: 'wifi' },
      { kind: 'function', name: 'wifi_connect' },
      { kind: 'function', name: 'wifi_disconnect' },
      { kind: 'function', name: 'set_base_url' },
      { kind: 'function', name: 'base_url' },
      { kind: 'function', name: 'fetch' },
    ],
  },
]

/** Public Berry globals registered via `be_regfunc` for COS scripts. */
export const cosGlobalFunctions: readonly GlobalFunctionInventory[] = [
  {
    name: 'log',
    source: 'components/berry_lang/src/berry_runtime.cpp',
    declarationPattern: 'function log(',
  },
  {
    name: 'action',
    source: 'components/berry_lang/src/berry_runtime.cpp',
    declarationPattern: 'function action(',
  },
]

/**
 * Berry built-in modules exposed through `@cos/*`/`@berry/*` and present in the
 * SDK module declarations. Their detailed API is owned by Berry/upstream docs;
 * R11 validates declaration presence and resolver aliases, not every stdlib member.
 */
export const berryBuiltinModuleSpecifiers = {
  string: ['@cos/string', '@berry/string'],
  json: ['@cos/json', '@berry/json'],
  math: ['@cos/math', '@berry/math'],
  global: ['@cos/global', '@berry/global'],
  gc: ['@cos/gc', '@berry/gc'],
  introspect: ['@cos/introspect', '@berry/introspect'],
} as const

export function publicBindings(module: NativeModuleInventory): readonly Binding[] {
  return module.bindings.filter((binding) => !binding.internal)
}
