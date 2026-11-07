import { describe, expect, it } from "vitest";

import {
  berryPathFor,
  canOpenPath,
  extensionOf,
  fileStem,
  isTypeScriptPath,
  joinPath,
  languageForPath,
  parentPath,
} from "./paths";

describe("paths", () => {
  it("maps source extensions to editor languages", () => {
    expect(languageForPath("/flash/app.ts")).toBe("typescript");
    expect(languageForPath("/flash/app.tsx")).toBe("typescript");
    expect(languageForPath("widget.mjs")).toBe("javascript");
    expect(languageForPath("/flash/notes.md")).toBe("markdown");
    expect(languageForPath("/flash/main.be")).toBe("plaintext");
    expect(languageForPath("/flash/script.berry")).toBe("plaintext");
    expect(languageForPath("/flash/README")).toBe("plaintext");
  });

  it("rejects binary files and accepts text", () => {
    expect(canOpenPath("/flash/icon.png")).toBe(false);
    expect(canOpenPath("/flash/app.ts")).toBe(true);
    expect(canOpenPath("/flash/autoexec.be")).toBe(true);
    expect(extensionOf(".gitignore")).toBe("");
  });

  it("builds a sibling Berry path for TypeScript files", () => {
    expect(isTypeScriptPath("/flash/apps/counter.ts")).toBe(true);
    expect(berryPathFor("/flash/apps/counter.ts")).toBe("/flash/apps/counter.be");
    expect(berryPathFor("\\flash\\apps\\counter.tsx")).toBe("\\flash\\apps\\counter.be");
    expect(berryPathFor("/flash/app.js")).toBeNull();
    expect(fileStem("/flash/app.d.ts")).toBe("app.d");
  });

  it("walks virtual directories without leaving the roots", () => {
    expect(parentPath("/flash/apps/demo")).toBe("/flash/apps");
    expect(parentPath("/flash")).toBeNull();
    expect(parentPath("/sdcard")).toBeNull();
    expect(joinPath("/flash", "app.ts")).toBe("/flash/app.ts");
    expect(() => joinPath("/flash", "..")).toThrow("Invalid entry name");
  });
});
