import { isSafeEntryName } from "../ide/files/paths";

/** One file or directory gathered from a drop. */
export interface DropEntry {
  name: string;
  isFile: boolean;
  isDirectory: boolean;
  file: () => Promise<Blob>;
  children: () => Promise<DropEntry[]>;
}

/** Files and directories collected from a drop, with paths relative to the drop root. */
export interface CollectedDrop {
  directories: string[];
  files: { relativePath: string; blob: Blob; fileName: string }[];
}

/** Walk dropped entries. Names that cannot be joined onto a device path are skipped. */
export async function collectDrop(entries: readonly DropEntry[]): Promise<CollectedDrop> {
  const directories: string[] = [];
  const files: CollectedDrop["files"] = [];

  const visit = async (entry: DropEntry, prefix: string): Promise<void> => {
    if (!isSafeEntryName(entry.name)) {
      return;
    }
    const relativePath = prefix ? `${prefix}/${entry.name}` : entry.name;
    if (entry.isDirectory) {
      directories.push(relativePath);
      for (const child of await entry.children()) {
        await visit(child, relativePath);
      }
      return;
    }
    if (entry.isFile) {
      files.push({ relativePath, blob: await entry.file(), fileName: entry.name });
    }
  };

  for (const entry of entries) {
    await visit(entry, "");
  }
  return { directories, files };
}

/** Read a browser drop into {@link DropEntry} values. Folder structure is kept when the browser exposes it. */
export function entriesFromDataTransfer(data: DataTransfer): DropEntry[] {
  const fromItems: DropEntry[] = [];
  for (const item of data.items) {
    const entry = item.webkitGetAsEntry();
    if (entry) {
      fromItems.push(entryFromWebkit(entry));
    }
  }
  if (fromItems.length > 0) {
    return fromItems;
  }
  return [...data.files].map(entryFromFile);
}

function entryFromFile(file: File): DropEntry {
  return {
    name: file.name,
    isFile: true,
    isDirectory: false,
    file: () => Promise.resolve(file),
    children: () => Promise.resolve([]),
  };
}

function entryFromWebkit(entry: FileSystemEntry): DropEntry {
  return {
    name: entry.name,
    isFile: entry.isFile,
    isDirectory: entry.isDirectory,
    file: () =>
      new Promise((resolve, reject) => {
        if (!entry.isFile) {
          reject(new Error("Not a file"));
          return;
        }
        (entry as FileSystemFileEntry).file(resolve, reject);
      }),
    children: async () => {
      if (!entry.isDirectory) {
        return [];
      }
      const reader = (entry as FileSystemDirectoryEntry).createReader();
      const children: DropEntry[] = [];
      for (;;) {
        const batch = await new Promise<FileSystemEntry[]>((resolve, reject) => {
          reader.readEntries(resolve, reject);
        });
        if (batch.length === 0) {
          break;
        }
        children.push(...batch.map(entryFromWebkit));
      }
      return children;
    },
  };
}
