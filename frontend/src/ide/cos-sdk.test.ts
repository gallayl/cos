import ts from '@typescript/typescript6'
import { describe, expect, it } from 'vitest'

import berryBuiltinsTypes from '../cos-sdk/berry-builtins.d.ts?raw' with { type: 'text' }
import consoleTypes from '../cos-sdk/console.d.ts?raw' with { type: 'text' }
import filesystemTypes from '../cos-sdk/filesystem.d.ts?raw' with { type: 'text' }
import indexTypes from '../cos-sdk/index.d.ts?raw' with { type: 'text' }
import jsonTypes from '../cos-sdk/json.d.ts?raw' with { type: 'text' }
import languageTypes from '../cos-sdk/language.d.ts?raw' with { type: 'text' }
import networkTypes from '../cos-sdk/network.d.ts?raw' with { type: 'text' }
import uiTypes from '../cos-sdk/ui.d.ts?raw' with { type: 'text' }

const sdkRoot = 'file:///cos-sdk'
const appFile = 'file:///app.ts'

const sdkFiles = new Map<string, string>([
  [`${sdkRoot}/language.d.ts`, languageTypes],
  [`${sdkRoot}/ui.d.ts`, uiTypes],
  [`${sdkRoot}/console.d.ts`, consoleTypes],
  [`${sdkRoot}/filesystem.d.ts`, filesystemTypes],
  [`${sdkRoot}/network.d.ts`, networkTypes],
  [`${sdkRoot}/json.d.ts`, jsonTypes],
  [`${sdkRoot}/berry-builtins.d.ts`, berryBuiltinsTypes],
  [`${sdkRoot}/index.d.ts`, indexTypes],
])

function diagnosticsFor(source: string): { app: string[]; sdk: string[] } {
  const files = new Map(sdkFiles)
  files.set(appFile, source)
  const host: ts.CompilerHost = {
    getSourceFile(fileName) {
      const text = files.get(fileName)
      if (text === undefined) return undefined
      return ts.createSourceFile(fileName, text, ts.ScriptTarget.ESNext, true, ts.ScriptKind.TS)
    },
    getDefaultLibFileName: () => 'lib.d.ts',
    writeFile() {},
    getCurrentDirectory: () => '/',
    getCanonicalFileName: (fileName) => fileName,
    useCaseSensitiveFileNames: () => true,
    getNewLine: () => '\n',
    fileExists: (fileName) => files.has(fileName),
    readFile: (fileName) => files.get(fileName),
  }
  const program = ts.createProgram({
    rootNames: [appFile, ...sdkFiles.keys()],
    options: {
      noLib: true,
      strict: true,
      target: ts.ScriptTarget.ESNext,
      module: ts.ModuleKind.ESNext,
    },
    host,
  })
  const app: string[] = []
  const sdk: string[] = []
  for (const diagnostic of ts.getPreEmitDiagnostics(program)) {
    const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, '\n')
    const where = diagnostic.file ? `${diagnostic.file.fileName}: ${message}` : message
    if (diagnostic.file?.fileName === appFile) app.push(message)
    else sdk.push(where)
  }
  return { app, sdk }
}

const counter = `export default class Counter {
  private ticks = 0;

  setup(content: unknown, width: number, height: number): void {
    ui.timer(1000, () => this.tick());
  }

  private tick(): void {
    this.ticks += 1;
    log("ticks: " + this.ticks);
  }
}
`

describe('COS TypeScript SDK', () => {
  it('accepts the editor sample and the device APIs', () => {
    const result = diagnosticsFor(`
      ${counter}

      export class Tap implements CosApp {
        private target: ui.ButtonHandle | null = null;

        setup(content: unknown, width: number, height: number): void {
          this.target = ui.button(0, "TAP", 0, 0, 40, 20);
          if (this.target !== null) ui.on_click(this.target, () => this.hit());
          const listing = fs.list("/flash");
          if (listing.ok) log(listing.entries[0].name);
          else log(listing.error);
          const ping = net.ping("example.com", 1000);
          if (ping.ok) log(ping.address);
          console.read_line("Name: ", (line) => log(line === null ? "cancelled" : line));
          console.read_key((key) => log(key === null ? "cancelled" : key));
          console.read_char((character) => log(character === "y" ? "yes" : "no"));
          ui.fill_screen(ui.color565(10, 16, 34));
          if (ui.key_down(ui.KEY_LEFT)) action("clear");
        }

        private hit(): void {
          log("tap");
        }
      }
    `)
    expect(result.sdk).toEqual([])
    expect(result.app).toEqual([])
  })

  it('accepts a character-grid console application', () => {
    const result = diagnosticsFor(`
      const screen = console.size();
      if (typeof screen !== "string") {
        console.clear();
        console.set_font_size(2);
        console.move(0, 0);
        console.write("Score", "green");
        console.write_at(0, 1, "Quit? y/n", "yellow");
        const cursor = console.cursor();
        if (typeof cursor !== "string") log(cursor.x + "," + cursor.y);
        log(console.font_size() === 2 ? "large" : "small");
      }
      console.read_key((key) => {
        if (key === "up" || key === "down") console.write_at(0, 2, key);
      });
    `)
    expect(result.sdk).toEqual([])
    expect(result.app).toEqual([])
  })

  it('rejects JavaScript defaults and mismatched widget handles', () => {
    const result = diagnosticsFor(`
      console.log("nope");
      setTimeout(() => {}, 1);
      const pending: Promise<void> = Promise.resolve();
      Math.random();
      const label = ui.label(0, "Score", 0, 0, 10, 10);
      if (label !== null) ui.on_click(label, () => {});
      log(pending);
    `)
    const messages = result.app.join('\n')
    expect(result.sdk).toEqual([])
    expect(messages).toContain('console')
    expect(messages).toContain('setTimeout')
    expect(messages).toContain('Promise')
    expect(messages).toContain('Math')
    expect(messages).toContain('ButtonHandle')
  })

  it('accepts Berry print, str, and list push', () => {
    const result = diagnosticsFor(`
      function main(args: string[]): number {
        const items: number[] = [];
        items.push(args.length);
        print(str(args.length) + ": " + args[0]);
        return 0;
      }
    `)
    expect(result.sdk).toEqual([])
    expect(result.app).toEqual([])
  })

  it('accepts filesystem reads, writes, and relative paths', () => {
    const result = diagnosticsFor(`
      function main(args: string[]): number {
        const written = fs.write_text("./note.txt", "hello");
        if (written !== true) return 1;
        const text = fs.read_text("./note.txt");
        if (!text.ok) return 1;
        const saved = fs.write_json("./note.json", {
          label: "cos",
          flags: [true, false],
          child: { ok: true },
        });
        if (saved !== true) return 1;
        const loaded = fs.read_json("/flash/note.json");
        if (!loaded.ok) return 1;
        const record = loaded.value as { label: string };
        if (!fs.exists("./note.txt") || !fs.is_file("./note.txt") || fs.is_dir("./../")) return 1;
        const bytes = fs.size("./note.txt");
        if (!bytes.ok) return 1;
        log(text.text + record.label + bytes.size + args.length);
        return 0;
      }
    `)
    expect(result.sdk).toEqual([])
    expect(result.app).toEqual([])
  })

  it('type-checks for-of over arrays without a DOM library', () => {
    const result = diagnosticsFor(`
      const values = [1, 2, 3];
      let total = 0;
      for (const value of values) total += value;
      log("total " + total + " len " + values.length);
    `)
    expect(result.sdk).toEqual([])
    expect(result.app).toEqual([])
  })
})
