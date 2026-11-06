import { describe, expect, it, vi } from "vitest";

import {
  CosFilesError,
  basicAuthorization,
  describeCosError,
  listDirectory,
  readTextFile,
  writeTextFile,
} from "./cos-files";

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

describe("cos files", () => {
  it("lists a directory with directories first", async () => {
    const fetchImpl = vi.fn(async (_url: string, _init?: RequestInit) =>
      jsonResponse([
        { name: "b.txt", size: 2, is_dir: false },
        { name: "apps", size: 0, is_dir: true },
        { name: "a.txt", size: 1, is_dir: false },
        { name: "../secret", size: 1, is_dir: false },
      ]),
    );

    const entries = await listDirectory("/flash", {
      fetchImpl: fetchImpl as typeof fetch,
      authorization: "Basic abc",
    });

    expect(fetchImpl).toHaveBeenCalledWith("/api/files?path=/flash", expect.anything());
    const init = fetchImpl.mock.calls[0]?.[1];
    expect(new Headers(init?.headers).get("Authorization")).toBe("Basic abc");
    expect(entries.map((entry) => entry.name)).toEqual(["apps", "a.txt", "b.txt"]);
  });

  it("reads file text", async () => {
    const fetchImpl = vi.fn(async () => new Response("export default class App {}\n", { status: 200 }));
    const text = await readTextFile("/flash/app.ts", { fetchImpl: fetchImpl as typeof fetch });
    expect(text).toContain("class App");
    expect(fetchImpl).toHaveBeenCalledWith("/api/file?path=/flash/app.ts", expect.anything());
  });

  it("uploads text with the path field before the file", async () => {
    const fetchImpl = vi.fn(async (_url: string, init?: RequestInit) => {
      const body = init?.body as FormData;
      const names = [...body.keys()];
      expect(names).toEqual(["path", "file"]);
      expect(body.get("path")).toBe("/flash/app.ts");
      const file = body.get("file");
      expect(file).toBeInstanceOf(Blob);
      expect(await (file as Blob).text()).toBe("let ticks = 0\n");
      return jsonResponse({ status: "ok" });
    });

    await writeTextFile("/flash/app.ts", "let ticks = 0\n", { fetchImpl: fetchImpl as typeof fetch });
  });

  it("rejects a file list that is not JSON", async () => {
    const fetchImpl = vi.fn(async () => new Response("<!doctype html>", { status: 200 }));
    await expect(listDirectory("/flash", { fetchImpl: fetchImpl as typeof fetch })).rejects.toMatchObject({
      message: "Device file list is unavailable",
    });
  });

  it("reports device errors and authentication failures", async () => {
    const fetchImpl = vi.fn(async () => new Response("Unauthorized", { status: 401 }));
    await expect(readTextFile("/flash/app.ts", { fetchImpl: fetchImpl as typeof fetch })).rejects.toMatchObject({
      message: "Unauthorized",
      status: 401,
    });

    const described = describeCosError(new CosFilesError("Not found", 404));
    expect(described).toBe("Not found");
    expect(describeCosError(new TypeError("Failed to fetch"))).toBe("Cannot reach the device");
    expect(basicAuthorization("cos", "secret")).toBe(`Basic ${btoa("cos:secret")}`);
  });
});
