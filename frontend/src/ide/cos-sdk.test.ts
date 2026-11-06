import ts from "@typescript/typescript6";
import { describe, expect, it } from "vitest";

import consoleTypes from "../cos-sdk/console.d.ts?raw" with { type: "text" };
import filesystemTypes from "../cos-sdk/filesystem.d.ts?raw" with { type: "text" };
import indexTypes from "../cos-sdk/index.d.ts?raw" with { type: "text" };
import languageTypes from "../cos-sdk/language.d.ts?raw" with { type: "text" };
import networkTypes from "../cos-sdk/network.d.ts?raw" with { type: "text" };
import uiTypes from "../cos-sdk/ui.d.ts?raw" with { type: "text" };

const sdkRoot = "file:///cos-sdk";
const appFile = "file:///app.ts";

const sdkFiles = new Map<string, string>([
  [`${sdkRoot}/language.d.ts`, languageTypes],
  [`${sdkRoot}/ui.d.ts`, uiTypes],
  [`${sdkRoot}/console.d.ts`, consoleTypes],
  [`${sdkRoot}/filesystem.d.ts`, filesystemTypes],
  [`${sdkRoot}/network.d.ts`, networkTypes],
  [`${sdkRoot}/index.d.ts`, indexTypes],
]);

function diagnosticsFor(source: string): { app: string[]; sdk: string[] } {
  const files = new Map(sdkFiles);
  files.set(appFile, source);
  const host: ts.CompilerHost = {
    getSourceFile(fileName) {
      const text = files.get(fileName);
      if (text === undefined) return undefined;
      return ts.createSourceFile(fileName, text, ts.ScriptTarget.ESNext, true, ts.ScriptKind.TS);
    },
    getDefaultLibFileName: () => "lib.d.ts",
    writeFile() {},
    getCurrentDirectory: () => "/",
    getCanonicalFileName: (fileName) => fileName,
    useCaseSensitiveFileNames: () => true,
    getNewLine: () => "\n",
    fileExists: (fileName) => files.has(fileName),
    readFile: (fileName) => files.get(fileName),
  };
  const program = ts.createProgram({
    rootNames: [appFile, ...sdkFiles.keys()],
    options: {
      noLib: true,
      strict: true,
      target: ts.ScriptTarget.ESNext,
      module: ts.ModuleKind.ESNext,
    },
    host,
  });
  const app: string[] = [];
  const sdk: string[] = [];
  for (const diagnostic of ts.getPreEmitDiagnostics(program)) {
    const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n");
    const where = diagnostic.file ? `${diagnostic.file.fileName}: ${message}` : message;
    if (diagnostic.file?.fileName === appFile) app.push(message);
    else sdk.push(where);
  }
  return { app, sdk };
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
`;

describe("COS TypeScript SDK", () => {
  it("accepts the editor sample and the device APIs", () => {
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
          ui.fill_screen(ui.color565(10, 16, 34));
          if (ui.key_down(ui.KEY_LEFT)) action("clear");
        }

        private hit(): void {
          log("tap");
        }
      }
    `);
    expect(result.sdk).toEqual([]);
    expect(result.app).toEqual([]);
  });

  it("rejects JavaScript defaults and mismatched widget handles", () => {
    const result = diagnosticsFor(`
      console.log("nope");
      setTimeout(() => {}, 1);
      const pending: Promise<void> = Promise.resolve();
      Math.random();
      const label = ui.label(0, "Score", 0, 0, 10, 10);
      if (label !== null) ui.on_click(label, () => {});
      log(pending);
    `);
    const messages = result.app.join("\n");
    expect(result.sdk).toEqual([]);
    expect(messages).toContain("console");
    expect(messages).toContain("setTimeout");
    expect(messages).toContain("Promise");
    expect(messages).toContain("Math");
    expect(messages).toContain("ButtonHandle");
  });

  it("type-checks for-of over arrays without a DOM library", () => {
    const result = diagnosticsFor(`
      const values = [1, 2, 3];
      let total = 0;
      for (const value of values) total += value;
      log("total " + total + " len " + values.length);
    `);
    expect(result.sdk).toEqual([]);
    expect(result.app).toEqual([]);
  });
});
