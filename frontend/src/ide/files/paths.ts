const typescriptExtensions = new Set(["ts", "tsx", "mts", "cts"]);

const binaryExtensions = new Set([
  "png",
  "jpg",
  "jpeg",
  "gif",
  "webp",
  "ico",
  "bmp",
  "woff",
  "woff2",
  "ttf",
  "otf",
  "eot",
  "bin",
  "zip",
  "gz",
  "mp3",
  "wav",
  "ogg",
  "pdf",
]);

const roots = new Set(["/flash", "/sdcard"]);

/** File extension without the dot, or an empty string when there is none. */
export function extensionOf(path: string): string {
  const name = path.slice(Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\")) + 1);
  const dot = name.lastIndexOf(".");
  if (dot <= 0) {
    return "";
  }
  return name.slice(dot + 1).toLowerCase();
}

/** Monaco language id for a source path. Unknown text files use plaintext. */
export function languageForPath(path: string): string {
  switch (extensionOf(path)) {
    case "ts":
    case "tsx":
    case "mts":
    case "cts":
      return "typescript";
    case "js":
    case "jsx":
    case "mjs":
    case "cjs":
      return "javascript";
    case "json":
      return "json";
    case "css":
      return "css";
    case "html":
    case "htm":
      return "html";
    case "md":
    case "markdown":
      return "markdown";
    default:
      return "plaintext";
  }
}

/** True when the editor can show the file as text. */
export function canOpenPath(path: string): boolean {
  return !binaryExtensions.has(extensionOf(path));
}

/** True for TypeScript sources that can be compiled to Berry. */
export function isTypeScriptPath(path: string): boolean {
  return typescriptExtensions.has(extensionOf(path));
}

/** Final path segment, or `script.txt` when the path has no name. */
export function fileNameOf(path: string): string {
  const name = path.slice(Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\")) + 1);
  return name.length > 0 ? name : "script.txt";
}

/** File name without its last extension. */
export function fileStem(path: string): string {
  const name = fileNameOf(path);
  const dot = name.lastIndexOf(".");
  if (dot <= 0) {
    return name;
  }
  return name.slice(0, dot);
}

/**
 * Device path of the Berry script generated from a TypeScript file.
 * Returns null when `path` is not a TypeScript source.
 */
export function berryPathFor(path: string): string | null {
  if (!isTypeScriptPath(path)) {
    return null;
  }
  const slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"));
  const prefix = slash >= 0 ? path.slice(0, slash + 1) : "";
  return `${prefix}${fileStem(path)}.be`;
}

/** Parent directory, or null at `/flash` and `/sdcard`. */
export function parentPath(path: string): string | null {
  if (roots.has(path)) {
    return null;
  }
  const slash = path.lastIndexOf("/");
  if (slash <= 0) {
    return null;
  }
  return path.slice(0, slash);
}

/** Join a directory entry name onto a virtual directory path. */
export function joinPath(directory: string, name: string): string {
  if (!isSafeEntryName(name)) {
    throw new Error("Invalid entry name");
  }
  return `${directory}/${name}`;
}

/** True when a directory listing name can be joined onto a path. */
export function isSafeEntryName(name: string): boolean {
  return name !== "" && name !== "." && name !== ".." && !name.includes("/") && !name.includes("\\");
}
