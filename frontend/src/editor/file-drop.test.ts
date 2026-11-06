import { describe, expect, it } from "vitest";

import { collectDrop, type DropEntry } from "./file-drop";

describe("dropped files", () => {
  it("keeps a folder structure and skips unsafe names", () => {
    const tree = directory("notes", [
      file("todo.txt", "buy milk"),
      directory("skip/me", [file("secret.txt", "nope")]),
      file("..", "nope"),
    ]);

    return expect(collectDrop([tree, file("alone.txt", "hi")])).resolves.toEqual({
      directories: ["notes"],
      files: [
        { relativePath: "notes/todo.txt", blob: expect.any(Blob), fileName: "todo.txt" },
        { relativePath: "alone.txt", blob: expect.any(Blob), fileName: "alone.txt" },
      ],
    });
  });
});

function file(name: string, text: string): DropEntry {
  return {
    name,
    isFile: true,
    isDirectory: false,
    file: () => Promise.resolve(new Blob([text])),
    children: () => Promise.resolve([]),
  };
}

function directory(name: string, children: DropEntry[]): DropEntry {
  return {
    name,
    isFile: false,
    isDirectory: true,
    file: () => Promise.reject(new Error("Not a file")),
    children: () => Promise.resolve(children),
  };
}
