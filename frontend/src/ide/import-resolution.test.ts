import { describe, expect, it, vi } from "vitest";

import { fetchReferencedWorkspaceTypes, resolveImportPath } from "./import-resolution";

describe("resolveImportPath", () => {
  it("resolves explicit TS imports and adds .ts when missing", () => {
    expect(resolveImportPath("/flash", "./types.ts")).toBe("/flash/types.ts");
    expect(resolveImportPath("/flash/app", "../types")).toBe("/flash/types.ts");
    expect(resolveImportPath("/flash", "./ui/component.tsx")).toBe("/flash/ui/component.tsx");
  });

  it("rejects non-TypeScript extensions", () => {
    expect(resolveImportPath("/flash", "./data.json")).toBeNull();
    expect(resolveImportPath("/flash", "./style.css")).toBeNull();
  });
});

describe("fetchReferencedWorkspaceTypes", () => {
  it("fetches and registers device TypeScript imports", async () => {
    const fetcher = {
      get: vi.fn(async (path: string) => `// ${path}`),
    };
    const registerFile = vi.fn();

    await fetchReferencedWorkspaceTypes({
      documentPath: "/flash/app/main.ts",
      source: `
        import type { A } from "./types";
        import { b } from "../shared.ts";
        import "./types";
      `,
      authorization: "Basic abc",
      fetcher,
      registerFile,
    });

    expect(fetcher.get).toHaveBeenCalledTimes(2);
    expect(fetcher.get).toHaveBeenCalledWith("/flash/app/types.ts", "Basic abc");
    expect(fetcher.get).toHaveBeenCalledWith("/flash/shared.ts", "Basic abc");
    expect(registerFile).toHaveBeenCalledTimes(2);
  });

  it("ignores non-device paths and fetch failures", async () => {
    const fetcher = {
      get: vi.fn(async (path: string) => {
        if (path.endsWith("missing.ts")) throw new Error("404");
        return "export {}";
      }),
    };
    const registerFile = vi.fn();

    await fetchReferencedWorkspaceTypes({
      documentPath: "/tmp/main.ts",
      source: `import "./types";`,
      authorization: "",
      fetcher,
      registerFile,
    });
    expect(fetcher.get).not.toHaveBeenCalled();

    await fetchReferencedWorkspaceTypes({
      documentPath: "/flash/main.ts",
      source: `import "./missing"; import "./ok";`,
      authorization: "",
      fetcher,
      registerFile,
    });

    expect(registerFile).toHaveBeenCalledTimes(1);
    expect(registerFile).toHaveBeenCalledWith("/flash/ok.ts", "export {}");
  });

  it("does nothing without a document directory", async () => {
    const fetcher = { get: vi.fn(async () => "") };
    const registerFile = vi.fn();

    await fetchReferencedWorkspaceTypes({
      documentPath: "script.ts",
      source: `import "./types";`,
      authorization: "",
      fetcher,
      registerFile,
    });

    expect(fetcher.get).not.toHaveBeenCalled();
    expect(registerFile).not.toHaveBeenCalled();
  });
});
