import { describe, expect, it } from "vitest";

import { main, verifyStorage } from "./compile-storage";

describe("storage TypeScript scripts", () => {
  const report = verifyStorage();

  it("uses the COS SDK without JavaScript libraries", () => {
    expect(report.optionProblems).toEqual([]);
    const messages = report.nonSdk.join("\n");
    expect(messages).toContain("console");
    expect(messages).toContain("setTimeout");
    expect(messages).toContain("Math");
  });

  it("typechecks every storage script", () => {
    expect(report.typecheck).toEqual([]);
  });

  it("compiles every script to the device Berry program", () => {
    expect(main()).toBe(0);
    expect(report.scripts.map((script) => script.name)).toEqual([
      "apps/breakout",
      "apps/tap_dash",
      "autoexec",
      "scripts/arg_test",
      "scripts/clear",
      "scripts/hello",
      "scripts/list_files",
      "scripts/ping",
      "scripts/prompt",
    ]);
    for (const script of report.scripts) {
      expect(script.diagnostics, script.name).toEqual([]);
      expect(script.diff, script.name).toBe("");
    }
  });
});
