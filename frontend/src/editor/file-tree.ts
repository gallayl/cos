import { ObservableValue } from "@furystack/utils";

import type { CosDirEntry } from "../ide/files/cos-files";
import { isSafeEntryName, joinPath } from "../ide/files/paths";

/** Row shown in the device file tree. */
export type FileNodeKind = "directory" | "file" | "pending" | "error" | "draft";

/** One stable tree row. Directory listings reuse the same object for a path. */
export interface FileNode {
  kind: FileNodeKind;
  name: string;
  path: string;
  parentPath: string;
  editing: boolean;
  draftKind?: "file" | "directory";
  message?: string;
}

/** Context-menu command for a tree row. */
export type FileMenuAction = "new-file" | "new-folder" | "rename" | "delete";

const filesystemRoots = ["/flash", "/sdcard"] as const;

/** True for `/flash` and `/sdcard`. Those rows cannot be renamed or deleted. */
export function isFilesystemRoot(path: string): boolean {
  return (filesystemRoots as readonly string[]).includes(path);
}

/** Message for a rejected file or folder name, or null when the name can be used. */
export function entryNameError(name: string): string | null {
  if (name.trim() !== name || !isSafeEntryName(name)) {
    return "Use a name without slashes";
  }
  return null;
}

/**
 * True when the file tree should drop its focused row.
 * A focused row calls `focus()` on every redraw, which pulls the caret out of the editor.
 * Monaco's caret target is a plain div, so this follows the element path instead of the tag.
 */
export function shouldReleaseTreeFocus(path: readonly { tagName?: string; isContentEditable?: boolean }[]): boolean {
  const inTree = path.some((entry) => entry.tagName === "COS-FILE-TREE-VIEW");
  if (!inTree) {
    return true;
  }
  return path.some(
    (entry) => entry.tagName === "INPUT" || entry.tagName === "TEXTAREA" || entry.isContentEditable === true,
  );
}

/** Character range selected when a rename starts. Files keep their extension. */
export function nameSelection(name: string, isDirectory: boolean): { start: number; end: number } {
  if (isDirectory || name.startsWith(".")) {
    return { start: 0, end: name.length };
  }
  const dot = name.lastIndexOf(".");
  if (dot <= 0) {
    return { start: 0, end: name.length };
  }
  return { start: 0, end: dot };
}

/**
 * Path after `from` is renamed to `to`.
 * Returns null when `path` is outside the renamed entry.
 */
export function movedPath(path: string, from: string, to: string): string | null {
  if (!path || path === from) {
    return path === from ? to : null;
  }
  if (path.startsWith(`${from}/`)) {
    return to + path.slice(from.length);
  }
  return null;
}

/** Join a relative drop path onto a virtual directory. Returns null for unsafe segments. */
export function childPath(directory: string, relativePath: string): string | null {
  const segments = relativePath.split("/").filter((segment) => segment.length > 0);
  if (segments.length === 0 || segments.some((segment) => !isSafeEntryName(segment))) {
    return null;
  }
  return segments.reduce((path, segment) => `${path}/${segment}`, directory);
}

/** Directories to create before uploading dropped files, shallowest first. */
export function directoriesToCreate(directories: readonly string[], files: readonly string[]): string[] {
  const collected = new Set(directories.filter((directory) => directory.length > 0));
  for (const file of files) {
    const parts = file.split("/");
    parts.pop();
    let prefix = "";
    for (const part of parts) {
      prefix = prefix ? `${prefix}/${part}` : part;
      if (prefix.length > 0) {
        collected.add(prefix);
      }
    }
  }
  return [...collected].sort(
    (left, right) => left.split("/").length - right.split("/").length || left.localeCompare(right),
  );
}

/** Commands offered for a row. Pending and error rows act on their parent directory. */
export function menuActions(node: FileNode | undefined): FileMenuAction[] {
  if (!node || node.kind === "pending" || node.kind === "error" || node.kind === "draft") {
    return ["new-file", "new-folder"];
  }
  if (node.kind === "directory") {
    if (isFilesystemRoot(node.path)) {
      return ["new-file", "new-folder"];
    }
    return ["new-file", "new-folder", "rename", "delete"];
  }
  return ["rename", "delete"];
}

/** Directory a create or drop command should write into. */
export function commandDirectory(node: FileNode | undefined, fallback = "/flash"): string {
  if (!node || node.kind === "pending" || node.kind === "error" || node.kind === "draft") {
    return node?.parentPath || fallback;
  }
  if (node.kind === "directory") {
    return node.path;
  }
  return node.parentPath || fallback;
}

function createNode(kind: FileNodeKind, name: string, path: string, parentPath: string): FileNode {
  return { kind, name, path, parentPath, editing: false };
}

/**
 * Loaded directory rows for the file tree.
 * Node objects stay stable across refreshes so expansion state can follow them.
 */
export class FileTreeStore {
  readonly revision = new ObservableValue(0);
  readonly roots: FileNode[];
  private readonly children = new Map<string, FileNode[]>();
  private readonly placeholders = new Map<string, FileNode>();

  constructor() {
    this.roots = filesystemRoots.map((path) => createNode("directory", path.slice(1), path, ""));
  }

  [Symbol.dispose](): void {
    this.revision[Symbol.dispose]();
  }

  /** Children already loaded, or a single pending row when the directory has not been listed. */
  getChildren = (node: FileNode): FileNode[] => {
    if (node.kind !== "directory") {
      return [];
    }
    return this.children.get(node.path) ?? [this.placeholder(node.path, "pending")];
  };

  findByPath(path: string): FileNode | undefined {
    const root = this.roots.find((node) => node.path === path);
    if (root) {
      return root;
    }
    for (const nodes of this.children.values()) {
      const found = nodes.find((node) => node.path === path);
      if (found) {
        return found;
      }
    }
    return undefined;
  }

  hasChildName(directory: string, name: string, ignore?: FileNode): boolean {
    return (this.children.get(directory) ?? []).some(
      (node) =>
        node !== ignore &&
        node.kind !== "draft" &&
        node.kind !== "pending" &&
        node.kind !== "error" &&
        node.name === name,
    );
  }

  /** Replace a directory listing. Draft rows and unchanged entry objects are kept. */
  setListing(directory: string, entries: readonly Pick<CosDirEntry, "name" | "is_dir">[]): void {
    const previous = new Map<string, FileNode>();
    for (const child of this.children.get(directory) ?? []) {
      if (child.kind === "file" || child.kind === "directory") {
        previous.set(child.path, child);
      }
    }
    const drafts = (this.children.get(directory) ?? []).filter((child) => child.kind === "draft");
    const next = [...drafts];
    for (const entry of entries) {
      const path = joinPath(directory, entry.name);
      const kind: FileNodeKind = entry.is_dir ? "directory" : "file";
      const existing = previous.get(path);
      if (existing && existing.kind === kind) {
        existing.name = entry.name;
        next.push(existing);
        continue;
      }
      next.push(createNode(kind, entry.name, path, directory));
    }
    this.children.set(directory, next);
    this.touch();
  }

  /** Show a failed listing when the directory has no entries yet. */
  setError(directory: string, message: string): void {
    const current = this.children.get(directory) ?? [];
    if (current.some((node) => node.kind === "file" || node.kind === "directory")) {
      return;
    }
    const drafts = current.filter((node) => node.kind === "draft");
    this.children.set(directory, [...drafts, this.placeholder(directory, "error", message)]);
    this.touch();
  }

  /** Drop loaded rows so the next expand fetches them again. */
  clearListings(): void {
    this.children.clear();
    this.placeholders.clear();
    this.touch();
  }

  /** Insert an inline create row at the top of a loaded directory. */
  insertDraft(directory: string, draftKind: "file" | "directory"): FileNode {
    this.clearEditing();
    const current = (this.children.get(directory) ?? []).filter(
      (node) => node.kind !== "draft" && node.kind !== "pending" && node.kind !== "error",
    );
    const draft = createNode("draft", "", `${directory}::draft`, directory);
    draft.draftKind = draftKind;
    draft.editing = true;
    this.children.set(directory, [draft, ...current]);
    this.touch();
    return draft;
  }

  beginRename(node: FileNode): void {
    this.clearEditing();
    node.editing = true;
    this.touch();
  }

  cancelEdit(): void {
    this.clearEditing();
    this.touch();
  }

  /**
   * Point a renamed entry and its loaded descendants at `nextName`.
   * Call this after the device accepts the rename.
   */
  applyRename(node: FileNode, nextName: string): { from: string; to: string } {
    const to = joinPath(node.parentPath, nextName);
    const from = node.path;
    if (to !== from) {
      this.reprefix(from, to);
      node.name = nextName;
    }
    node.editing = false;
    this.touch();
    return { from, to };
  }

  removeDraft(node: FileNode): void {
    const siblings = this.children.get(node.parentPath);
    if (!siblings) {
      return;
    }
    this.children.set(
      node.parentPath,
      siblings.filter((child) => child !== node),
    );
    this.touch();
  }

  /** Ask the tree to render the current rows again. */
  notify(): void {
    this.touch();
  }

  /** Remove a row and any loaded descendants. */
  removeLocal(node: FileNode): void {
    const siblings = this.children.get(node.parentPath);
    if (siblings) {
      this.children.set(
        node.parentPath,
        siblings.filter((child) => child !== node),
      );
    }
    this.forget(node.path);
    this.touch();
  }

  private clearEditing(): void {
    for (const [directory, nodes] of this.children) {
      const kept = nodes.filter((node) => node.kind !== "draft");
      for (const node of kept) {
        node.editing = false;
      }
      if (kept.length !== nodes.length) {
        this.children.set(directory, kept);
      }
    }
  }

  private forget(path: string): void {
    for (const key of [...this.children.keys()]) {
      if (key === path || key.startsWith(`${path}/`)) {
        this.children.delete(key);
      }
    }
    for (const key of [...this.placeholders.keys()]) {
      const directory = key.slice(key.indexOf(":") + 1);
      if (directory === path || directory.startsWith(`${path}/`)) {
        this.placeholders.delete(key);
      }
    }
  }

  private reprefix(from: string, to: string): void {
    const rewrite = (path: string): string => {
      if (path === from) {
        return to;
      }
      if (path.startsWith(`${from}/`)) {
        return to + path.slice(from.length);
      }
      return path;
    };
    for (const nodes of this.children.values()) {
      for (const node of nodes) {
        node.path = rewrite(node.path);
        node.parentPath = rewrite(node.parentPath);
      }
    }
    const moved = new Map<string, FileNode[]>();
    for (const [directory, nodes] of this.children) {
      moved.set(rewrite(directory), nodes);
    }
    this.children.clear();
    for (const [directory, nodes] of moved) {
      this.children.set(directory, nodes);
    }
    const movedPlaceholders = new Map<string, FileNode>();
    for (const [key, node] of this.placeholders) {
      const separator = key.indexOf(":");
      const kind = key.slice(0, separator);
      const directory = rewrite(key.slice(separator + 1));
      node.parentPath = directory;
      node.path = `${directory}::${kind}`;
      movedPlaceholders.set(`${kind}:${directory}`, node);
    }
    this.placeholders.clear();
    for (const [key, node] of movedPlaceholders) {
      this.placeholders.set(key, node);
    }
  }

  private placeholder(directory: string, kind: "pending" | "error", message?: string): FileNode {
    const key = `${kind}:${directory}`;
    const existing = this.placeholders.get(key);
    if (existing) {
      if (message) {
        existing.message = message;
        existing.name = message;
      }
      return existing;
    }
    const node = createNode(kind, message ?? "Loading", `${directory}::${kind}`, directory);
    node.message = message;
    this.placeholders.set(key, node);
    return node;
  }

  private touch(): void {
    this.revision.setValue(this.revision.getValue() + 1);
  }
}
