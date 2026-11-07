import { describe, expect, it } from "vitest";

import { astToBerry } from "./ast-to-berry";
import { getAst } from "./get-ast";

function compile(source: string) {
  return astToBerry(getAst(source));
}

function expectBerry(source: string, code: string) {
  const result = compile(source);
  expect(result.diagnostics).toEqual([]);
  expect(result.ok).toBe(true);
  expect(result.code).toBe(code);
}

describe("astToBerry", () => {
  it("lowers a default-exported COS application class", () => {
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
    );
  });

  it("lowers functions, branches, loops, and constructors", () => {
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
    );
  });

  it("lowers templates, nil, collections, length, and switch", () => {
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
    var __cos_switch_1 = items.size()
    if (__cos_switch_1 == 0)
        return "none"
    elif (__cos_switch_1 == 1) || (__cos_switch_1 == 2)
        return "Hello " + str(name) + "!"
    else
        return items[0]
    end
end
`,
    );
  });

  it("lowers block arrows and keeps sibling bindings distinct", () => {
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
    );
  });

  it("erases types and comments", () => {
    expectBerry(
      `
        interface Point { x: number }
        type Id = number;
        // kept out of the runtime
        const value: Id = 1;
      `,
      `var value = 1
`,
    );
  });

  it("erases type-only imports", () => {
    expectBerry(
      `
        import type { MyCustomType } from "./formSomewhere";
        import type * as Types from "./types";
        import type DefaultType from "./default";
        const value = 1;
      `,
      `var value = 1
`,
    );
  });

  it("returns an empty program for type-only source", () => {
    expectBerry(`interface Only { value: number }`, "");
    expectBerry(`import type { Only } from "./types";`, "");
  });

  it("reports syntax the runtime cannot represent", () => {
    const result = compile(`
      import { screen } from "./ui";
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
    `);

    expect(result.ok).toBe(false);
    const messages = result.diagnostics.map((diagnostic) => diagnostic.message);
    expect(messages).toEqual(
      expect.arrayContaining([
        "Imports are not supported. COS APIs are available as globals.",
        "Enums are not supported yet.",
        "Loose equality is not supported. Use === and !==.",
        "Conditions must be boolean. Compare values explicitly; Berry treats only false and nil as falsy.",
        'Redeclaring "i" is not supported.',
        "Single inheritance is not supported yet.",
        "Switch fallthrough is not supported.",
      ]),
    );
  });

  it("reports parse errors and emits no code", () => {
    const result = compile("const value =");
    expect(result.ok).toBe(false);
    expect(result.code).toBe("");
    expect(result.diagnostics[0]?.message).toContain("expected");
    expect(result.diagnostics[0]?.line).toBe(1);
  });
});
