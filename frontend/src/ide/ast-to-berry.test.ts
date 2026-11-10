import { describe, expect, it } from 'vitest'

import { astToBerry } from './ast-to-berry'
import { getAst } from './get-ast'

function compile(source: string) {
  return astToBerry(getAst(source), { entrypoint: false })
}

function compileEntrypoint(source: string) {
  return astToBerry(getAst(source))
}

function expectBerry(source: string, code: string) {
  const result = compile(source)
  expect(result.diagnostics).toEqual([])
  expect(result.ok).toBe(true)
  expect(result.code).toBe(code)
}

describe('astToBerry', () => {
  it('lowers a default-exported COS application class', () => {
    expectBerry(
      `
        export default class Counter {
          private ticks = 0;

          setup(content: unknown, width: number, height: number): void {
            ui.timer(1000, () => this.tick());
          }

          private tick(): void {
            this.ticks += 1;
            log("ticks: " + this.ticks);
          }
        }
      `,
      `class Counter
    var ticks
    def init()
        self.ticks = 0
    end
    def setup(content, width, height)
        ui.timer(1000, def () return self.tick() end)
    end
    def tick()
        self.ticks += 1
        log("ticks: " + self.ticks)
    end
end

return Counter
`,
    )
  })

  it('lowers functions, branches, loops, and constructors', () => {
    expectBerry(
      `
        class Box {
          value: number;
          constructor(value: number) {
            this.value = value;
          }
        }

        function sign(n: number): number {
          if (n > 0) {
            return 1;
          } else if (n < 0) {
            return -1;
          } else {
            return 0;
          }
        }

        function count(n: number): number {
          let total = 0;
          for (let i = 0; i < n; i++) {
            total += i;
          }
          return total;
        }

        function joined(items: string[]): string {
          let result = "";
          for (const item of items) {
            result += item;
          }
          return result;
        }

        function make(): Box {
          return new Box(1);
        }
      `,
      `class Box
    var value
    def init(value)
        self.value = value
    end
end
def sign(n)
    if n > 0
        return 1
    elif n < 0
        return -1
    else
        return 0
    end
end
def count(n)
    var total = 0
    var i = 0
    while i < n
        total += i
        i += 1
    end
    return total
end
def joined(items)
    var result = ""
    for item: items
        result += item
    end
    return result
end
def make()
    return Box(1)
end
`,
    )
  })

  it('lowers templates, nil, collections, length, and switch', () => {
    expectBerry(
      `
        function describe(name: string | null, items: string[]): string {
          if (name === null || name === undefined) {
            return "empty";
          }
          const point = { x: 1, y: 2 };
          switch (items.length) {
            case 0:
              return "none";
            case 1:
            case 2:
              return \`Hello \${name}!\`;
            default:
              return items[0];
          }
        }
      `,
      `def describe(name, items)
    if name == nil || name == nil
        return "empty"
    end
    var point = {"x": 1, "y": 2}
    var __cos_switch_1 = size(items)
    if (__cos_switch_1 == 0)
        return "none"
    elif (__cos_switch_1 == 1) || (__cos_switch_1 == 2)
        return "Hello " + str(name) + "!"
    else
        return items[0]
    end
end
`,
    )
  })

  it('lowers block arrows and keeps sibling bindings distinct', () => {
    expectBerry(
      `
        function run(): void {
          ui.timer(1000, () => {
            log("tick");
          });
          {
            const value = 1;
            log(value);
          }
          {
            const value = 2;
            log(value);
          }
        }
      `,
      `def run()
    ui.timer(1000, def ()
        log("tick")
    end)
    var value = 1
    log(value)
    var value = 2
    log(value)
end
`,
    )
  })

  it('erases types and comments', () => {
    expectBerry(
      `
        interface Point { x: number }
        type Id = number;
        // kept out of the runtime
        const value: Id = 1;
      `,
      `var value = 1
`,
    )
  })

  it('erases type-only imports', () => {
    expectBerry(
      `
        import type { MyCustomType } from "./formSomewhere";
        import type * as Types from "./types";
        import type DefaultType from "./default";
        const value = 1;
      `,
      `var value = 1
`,
    )
  })

  it('returns an empty program for type-only source', () => {
    expectBerry(`interface Only { value: number }`, '')
    expectBerry(`import type { Only } from "./types";`, '')
  })

  it('lowers default imports from relative modules', () => {
    expectBerry(
      `
        import myCustomStuff from "./myCustomModule";
        import Widget from "../ui/widget.ts";
        myCustomStuff();
        return Widget;
      `,
      `var __cos_mod_1 = cos_import("./myCustomModule")
var myCustomStuff = __cos_mod_1.find("default")
var __cos_mod_2 = cos_import("../ui/widget.ts")
var Widget = __cos_mod_2.find("default")
myCustomStuff()
return Widget
`,
    )
  })

  it('lowers named and namespace imports from relative modules', () => {
    expectBerry(
      `
        import { add, sub as minus } from "./math";
        import * as util from "./util";
        import main, { helper } from "./mixed";
        add(1, 2);
        minus(3, 1);
        util.run();
        main(helper);
      `,
      `var __cos_mod_1 = cos_import("./math")
var add = __cos_mod_1.find("add")
var minus = __cos_mod_1.find("sub")
var util = cos_import("./util")
var __cos_mod_2 = cos_import("./mixed")
var main = __cos_mod_2.find("default")
var helper = __cos_mod_2.find("helper")
add(1, 2)
minus(3, 1)
util.run()
main(helper)
`,
    )
  })

  it('lowers named exports as a module descriptor', () => {
    expectBerry(
      `
        export const answer = 42;
        export function add(a: number, b: number): number {
          return a + b;
        }
      `,
      `var answer = 42
def add(a, b)
    return a + b
end

return cos_module(nil, "answer", answer, "add", add)
`,
    )
  })

  it('lowers default and named exports together', () => {
    expectBerry(
      `
        export const version = 1;
        export default function main(): number {
          return version;
        }
      `,
      `var version = 1
def main()
    return version
end

return cos_module(main, "version", version)
`,
    )
  })

  it('lowers local re-export declarations', () => {
    expectBerry(
      `
        const internal = 1;
        function helper(): number {
          return internal;
        }
        export { internal as value, helper };
      `,
      `var internal = 1
def helper()
    return internal
end

return cos_module(nil, "value", internal, "helper", helper)
`,
    )
  })

  it('lowers relative module re-exports', () => {
    expectBerry(
      `
        export { add, sub as minus } from "./math";
        export * as math from "./math";
      `,
      `var __cos_re_1 = cos_import("./math")
var __cos_export_2 = __cos_re_1.find("add")
var __cos_export_3 = __cos_re_1.find("sub")
var __cos_re_4 = cos_import("./math")

return cos_module(nil, "add", __cos_export_2, "minus", __cos_export_3, "math", __cos_re_4)
`,
    )
  })

  it('keeps default-only app exports as raw returns', () => {
    expectBerry(
      `
        export default class Counter {
          setup(content: unknown, width: number, height: number): void {}
        }
      `,
      `class Counter
    def setup(content, width, height)
    end
end

return Counter
`,
    )
  })

  it('reports unsupported import and export forms', () => {
    const result = compile(`
      import "./side-effect";
      import { named } from "json";
      export * from "./all";
      export { x } from "@cos/x";
    `)

    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((diagnostic) => diagnostic.message)).toEqual([
      'Side-effect imports are not supported.',
      'Only relative imports and @cos/* module imports are supported.',
      'export * re-exports are not supported yet.',
    ])
  })

  it('reports syntax the runtime cannot represent', () => {
    const result = compile(`
      import "./ui";
      enum Color { Red }
      function same(a: number, b: number): boolean {
        return a == b;
      }
      function truthy(items: string[]): void {
        if (items.length) {
          log("yes");
        }
      }
      function shadow(i: number): number {
        for (let i = 0; i < 1; i++) {}
        return i;
      }
      class Child extends Base {}
      function fall(n: number): string {
        switch (n) {
          case 1:
            log("one");
          default:
            return "done";
        }
      }
    `)

    expect(result.ok).toBe(false)
    const messages = result.diagnostics.map((diagnostic) => diagnostic.message)
    expect(messages).toEqual(
      expect.arrayContaining([
        'Side-effect imports are not supported.',
        'Enums are not supported yet.',
        'Loose equality is not supported. Use === and !==.',
        'Conditions must be boolean. Compare values explicitly; Berry treats only false and nil as falsy.',
        'Redeclaring "i" is not supported.',
        'Class inheritance is not supported in Berry output.',
        'Switch fallthrough is not supported.',
      ]),
    )
  })

  it('lowers @cos module imports like relative imports', () => {
    expectBerry(
      `
        import net from "@cos/net";
        import { fetch } from "@cos/network";
        import * as ui from "@cos/ui";

        net.host();
        fetch("/ping");
        ui.label(0, "hi");
      `,
      `var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
var __cos_mod_2 = cos_import("@cos/network")
var __cos_def_3 = __cos_mod_2.find("default")
var fetch = __cos_def_3.fetch
var __cos_mod_4 = cos_import("@cos/ui")
var ui = __cos_mod_4.find("default")
net.host()
fetch("/ping")
ui.label(0, "hi")
`,
    )
  })

  it('reports parse errors and emits no code', () => {
    const result = compile('const value =')
    expect(result.ok).toBe(false)
    expect(result.code).toBe('')
    expect(result.diagnostics[0]?.message).toContain('expected')
    expect(result.diagnostics[0]?.line).toBe(1)
  })
})

describe('astToBerry try/catch/throw (R4)', () => {
  it('lowers try/catch with a bound exception variable', () => {
    expectBerry(
      `
        function parse(): string {
          try {
            return load();
          } catch (e) {
            log(e);
            return "fallback";
          }
        }
      `,
      `def parse()
    try
        return load()
    except .. as e
        log(e)
        return "fallback"
    end
end
`,
    )
  })

  it('lowers catch without a binding', () => {
    expectBerry(
      `
        function safe(): void {
          try {
            work();
          } catch {
            cleanup();
          }
        }
      `,
      `def safe()
    try
        work()
    except ..
        cleanup()
    end
end
`,
    )
  })

  it('lowers throw to Berry raise', () => {
    expectBerry(
      `
        function fail(message: string): void {
          throw message;
        }
      `,
      `def fail(message)
    raise message
end
`,
    )
  })

  it('lowers throw of a string literal', () => {
    expectBerry(`throw "stop_iteration";`, `raise "stop_iteration"\n`)
  })

  it('lowers nested try/catch and rethrow', () => {
    expectBerry(
      `
        function guarded(): void {
          try {
            try {
              risky();
            } catch (inner) {
              throw inner;
            }
          } catch (outer) {
            log(outer);
          }
        }
      `,
      `def guarded()
    try
        try
            risky()
        except .. as inner
            raise inner
        end
    except .. as outer
        log(outer)
    end
end
`,
    )
  })

  it('accepts typed catch bindings and erases the type', () => {
    expectBerry(
      `
        try {
          run();
        } catch (e: unknown) {
          handle(e);
        }
      `,
      `try
    run()
except .. as e
    handle(e)
end
`,
    )
  })

  it('lowers control flow inside try and catch blocks', () => {
    expectBerry(
      `
        function find(items: number[]): number {
          for (const item of items) {
            try {
              if (item > 0) {
                return item;
              }
              continue;
            } catch (e) {
              break;
            }
          }
          return -1;
        }
      `,
      `def find(items)
    for item: items
        try
            if item > 0
                return item
            end
            continue
        except .. as e
            break
        end
    end
    return -1
end
`,
    )
  })

  it('diagnoses try/finally because Berry has no finally clause', () => {
    const result = compile(`
      try {
        work();
      } finally {
        cleanup();
      }
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.message)).toEqual(
      expect.arrayContaining([expect.stringContaining('try/finally is not supported')]),
    )
  })

  it('diagnoses try/catch/finally', () => {
    const result = compile(`
      try {
        work();
      } catch (e) {
        handle(e);
      } finally {
        cleanup();
      }
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.message)).toEqual(
      expect.arrayContaining([expect.stringContaining('try/finally is not supported')]),
    )
  })

  it('diagnoses destructuring in catch bindings', () => {
    const result = compile(`
      try {
        work();
      } catch ({ message }) {
        log(message);
      }
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.message)).toEqual(
      expect.arrayContaining([expect.stringContaining('Destructuring in catch bindings is not supported')]),
    )
  })
})

describe('astToBerry semantic conformance (R3)', () => {
  it('lowers .length to Berry global size() for strings and containers', () => {
    expectBerry(
      `
        function lengths(text: string, items: number[]): number {
          const a = text.length;
          const b = items.length;
          return a + b;
        }
      `,
      `def lengths(text, items)
    var a = size(text)
    var b = size(items)
    return a + b
end
`,
    )
  })

  it('preserves explicit boolean comparisons and nil mappings', () => {
    expectBerry(
      `
        function ready(name: string | null, count: number): boolean {
          if (name !== null && name !== undefined && count > 0) {
            return true;
          }
          return false;
        }
      `,
      `def ready(name, count)
    if name != nil && name != nil && count > 0
        return true
    end
    return false
end
`,
    )
  })

  it('diagnoses integer literal division because Berry divides ints as integers', () => {
    const result = compile(`const ratio = 1 / 2;`)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.message)).toEqual(
      expect.arrayContaining([expect.stringContaining('Integer literal division uses Berry integer division')]),
    )
  })

  it('allows real-literal division and variable division', () => {
    expectBerry(`const a = 1.0 / 2;`, `var a = 1.0 / 2\n`)
    expectBerry(`const b = 1 / 2.0;`, `var b = 1 / 2.0\n`)
    expectBerry(
      `
        function half(n: number): number {
          return n / 2;
        }
      `,
      `def half(n)
    return n / 2
end
`,
    )
  })

  it('allows explicit numeric conversion around division', () => {
    expectBerry(`const ratio = int(1) / int(2);`, `var ratio = int(1) / int(2)\n`)
    expectBerry(`const ratio = real(1) / real(2);`, `var ratio = real(1) / real(2)\n`)
  })

  it('diagnoses unary + coercion', () => {
    const result = compile(`const n = +value;`)
    expect(result.ok).toBe(false)
    expect(result.diagnostics[0]?.message).toContain('Unary + is not supported')
  })

  it('diagnoses typeof/void/delete as unsupported Berry mappings', () => {
    const result = compile(`
      const t = typeof value;
      void value;
      delete obj.key;
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.message)).toEqual(
      expect.arrayContaining([
        expect.stringContaining('typeof is not supported'),
        expect.stringContaining('void operator is not supported'),
        expect.stringContaining('delete operator is not supported'),
      ]),
    )
  })

  it('preserves numeric literals and strips numeric separators', () => {
    expectBerry(`const n = 1_000;`, `var n = 1000\n`)
    expectBerry(`const r = 3.14;`, `var r = 3.14\n`)
    expectBerry(`const e = 1e3;`, `var e = 1e3\n`)
  })

  it('keeps class field initialization in init', () => {
    expectBerry(
      `
        export default class App {
          private count = 1;
          setup(): void {}
        }
      `,
      `class App
    var count
    def init()
        self.count = 1
    end
    def setup()
    end
end

return App
`,
    )
  })
})

describe('astToBerry R13/R21 entrypoints', () => {
  it('lowers a UI app entrypoint to __cos_entry (runtime-launched)', () => {
    const result = compileEntrypoint(`
      export default class Counter implements CosUiApplication {
        setup(_content: unknown, _width: number, _height: number): void {}
      }
    `)
    expect(result.ok).toBe(true)
    expect(result.code).toBe(`class Counter
    def setup(_content, _width, _height)
    end
end

var __cos_entry = Counter
`)
  })

  it('allows UI apps to declare run(args) for compatibility', () => {
    const result = compileEntrypoint(`
      export default class App implements CosUiApplication {
        run(_args: string[]): number { return 0 }
        setup(_content: unknown, _width: number, _height: number): void {}
      }
    `)
    expect(result.ok).toBe(true)
    expect(result.diagnostics).toEqual([])
    expect(result.code).toBe(`class App
    def run(_args)
        return 0
    end
    def setup(_content, _width, _height)
    end
end

var __cos_entry = App
`)
  })

  it('lowers a console app entrypoint to __cos_entry (runtime invokes run)', () => {
    const result = compileEntrypoint(`
      export default class Hello implements CosApplication {
        run(args: string[]): number {
          print(args.length)
          return 0
        }
      }
    `)
    expect(result.ok).toBe(true)
    expect(result.code).toBe(`class Hello
    def run(args)
        print(size(args))
        return 0
    end
end

var __cos_entry = Hello()
`)
  })

  it('requires a default exported app class in entrypoint mode', () => {
    const result = compileEntrypoint(`log("hello")`)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.code)).toEqual(
      expect.arrayContaining(['cos/no-top-level-exec', 'cos/default-class-entrypoint']),
    )
  })

  it('rejects module-scope variables and top-level functions in entrypoints', () => {
    const result = compileEntrypoint(`
      const x = 1
      function helper() {}
      export default class App implements CosApplication { run(): number { return x } }
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.code)).toEqual(
      expect.arrayContaining(['cos/no-module-state', 'cos/no-top-level-function']),
    )
  })

  it('rejects app classes without setup or run', () => {
    const result = compileEntrypoint(`
      export default class App implements CosApplication {}
    `)
    expect(result.ok).toBe(false)
    expect(result.diagnostics.map((d) => d.code)).toContain('cos/entrypoint-contract')
  })
})

describe('astToBerry R14 profiles and debug metadata', () => {
  it('defaults to dev profile metadata with statement mappings', () => {
    const result = compile(`const value = 1\nlog(value)`)
    expect(result.ok).toBe(true)
    expect(result.metadata?.profile).toBe('dev')
    expect((result.metadata?.mappings.length ?? 0) >= 2).toBe(true)
    expect(result.metadata?.notes?.length).toBeGreaterThan(0)
  })

  it('keeps generated Berry identical across dev and prod for supported input', () => {
    const source = `
      function greet(name: string): string {
        return "hi " + name
      }
      const msg = greet("cos")
      log(msg)
    `
    const dev = astToBerry(getAst(source), { profile: 'dev', entrypoint: false })
    const prod = astToBerry(getAst(source), { profile: 'prod', entrypoint: false })
    expect(dev.ok).toBe(true)
    expect(prod.ok).toBe(true)
    expect(prod.code).toBe(dev.code)
    expect(prod.metadata?.profile).toBe('prod')
    expect(prod.metadata?.mappings).toEqual([])
    expect((dev.metadata?.mappings.length ?? 0) > 0).toBe(true)
  })

  it('includes metadata on parse failures', () => {
    const result = astToBerry(getAst('const value ='), { profile: 'prod', entrypoint: false })
    expect(result.ok).toBe(false)
    expect(result.metadata?.profile).toBe('prod')
    expect(result.metadata?.mappings).toEqual([])
  })
})
