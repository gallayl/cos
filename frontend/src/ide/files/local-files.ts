interface FilePicker {
  showOpenFilePicker?: (options?: { multiple?: boolean }) => Promise<FileSystemFileHandle[]>;
  showSaveFilePicker?: (options?: { suggestedName?: string }) => Promise<FileSystemFileHandle>;
}

function filePicker(): FilePicker {
  return window as FilePicker;
}

/** The user closed a file dialog. */
export function isAbortError(error: unknown): boolean {
  return error instanceof DOMException && error.name === "AbortError";
}

/** True when this browser can open and save files without a download. */
export function supportsFilePicker(): boolean {
  const picker = filePicker();
  return typeof picker.showOpenFilePicker === "function" && typeof picker.showSaveFilePicker === "function";
}

/** Open one local text file. Returns null when the picker is unavailable. */
export async function pickTextFile(): Promise<{
  name: string;
  text: string;
  handle: FileSystemFileHandle;
} | null> {
  if (!supportsFilePicker()) {
    return null;
  }
  const open = filePicker().showOpenFilePicker;
  if (!open) {
    return null;
  }
  const [handle] = await open({ multiple: false });
  const file = await handle.getFile();
  return { name: file.name, text: await file.text(), handle };
}

/** Replace the contents of a file the user already granted access to. */
export async function writeTextToHandle(handle: FileSystemFileHandle, content: string): Promise<void> {
  const writable = await handle.createWritable();
  try {
    await writable.write(content);
  } finally {
    await writable.close();
  }
}

/**
 * Save text to a new local file.
 * Browsers without the file picker download the file instead and return null.
 */
export async function saveTextFile(suggestedName: string, content: string): Promise<FileSystemFileHandle | null> {
  if (!supportsFilePicker()) {
    downloadText(suggestedName, content);
    return null;
  }
  const save = filePicker().showSaveFilePicker;
  if (!save) {
    downloadText(suggestedName, content);
    return null;
  }
  const handle = await save({ suggestedName });
  await writeTextToHandle(handle, content);
  return handle;
}

/** Download text when the browser cannot write a chosen file directly. */
export function downloadText(filename: string, content: string): void {
  const url = URL.createObjectURL(new Blob([content], { type: "text/plain;charset=utf-8" }));
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename;
  document.body.appendChild(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
}
