import { isSafeEntryName } from "./paths";

/** One entry from `GET /api/files`. */
export interface CosDirEntry {
  name: string;
  size: number;
  is_dir: boolean;
}

/** Options for device filesystem requests. */
export interface CosRequestOptions {
  fetchImpl?: typeof fetch;
  /** Value of the Authorization header, including the `Basic ` prefix. */
  authorization?: string;
}

/** Failure reported by the device filesystem API. */
export class CosFilesError extends Error {
  readonly status: number;

  constructor(message: string, status: number) {
    super(message);
    this.name = "CosFilesError";
    this.status = status;
  }
}

/** `Authorization` header value for HTTP Basic credentials. */
export function basicAuthorization(username: string, password: string): string {
  return `Basic ${btoa(`${username}:${password}`)}`;
}

/** Short message for a failed filesystem call. */
export function describeCosError(error: unknown): string {
  if (error instanceof CosFilesError) {
    return error.message;
  }
  if (error instanceof Error) {
    if (error.message === "Failed to fetch") {
      return "Cannot reach the device";
    }
    return error.message;
  }
  return "Request failed";
}

/**
 * Encode a virtual path for a query string.
 * Slashes stay literal because the device query parser does not treat `%2F` as `/`.
 */
export function encodePathQuery(path: string): string {
  return encodeURIComponent(path).replaceAll("%2F", "/");
}

/** List a virtual directory. Directories are ordered before files. */
export async function listDirectory(path: string, options: CosRequestOptions = {}): Promise<CosDirEntry[]> {
  const response = await request(`/api/files?path=${encodePathQuery(path)}`, options);
  let body: unknown;
  try {
    body = await response.json();
  } catch {
    throw new CosFilesError("Device file list is unavailable", response.status);
  }
  if (!Array.isArray(body)) {
    throw new CosFilesError("Device file list is unavailable", response.status);
  }
  return body.filter(isDirEntry).sort(compareEntries);
}

/** Read a virtual file as text. */
export async function readTextFile(path: string, options: CosRequestOptions = {}): Promise<string> {
  const response = await request(`/api/file?path=${encodePathQuery(path)}`, options);
  return response.text();
}

/** Write a virtual file. Parent directories are created by the device. */
export async function writeTextFile(path: string, content: string, options: CosRequestOptions = {}): Promise<void> {
  await uploadFile(path, new Blob([content], { type: "text/plain" }), options);
}

/** Upload a file blob. Parent directories are created by the device. */
export async function uploadFile(path: string, content: Blob, options: CosRequestOptions = {}): Promise<void> {
  const body = new FormData();
  body.append("path", path);
  body.append("file", content, fileName(path));
  await request("/api/upload", options, { method: "POST", body });
}

/** Create an empty virtual directory. */
export async function createDirectory(path: string, options: CosRequestOptions = {}): Promise<void> {
  await request(`/api/mkdir?path=${encodePathQuery(path)}`, options, { method: "POST" });
}

/** Rename a file or directory on the same filesystem. */
export async function renameEntry(from: string, to: string, options: CosRequestOptions = {}): Promise<void> {
  await request(`/api/rename?from=${encodePathQuery(from)}&to=${encodePathQuery(to)}`, options, { method: "POST" });
}

/** Delete a file, or an empty directory. */
export async function deleteEntry(path: string, options: CosRequestOptions = {}): Promise<void> {
  await request(`/api/file?path=${encodePathQuery(path)}`, options, { method: "DELETE" });
}

function fileName(path: string): string {
  const name = path.slice(path.lastIndexOf("/") + 1);
  return name.length > 0 ? name : "file.txt";
}

async function request(url: string, options: CosRequestOptions, init: RequestInit = {}): Promise<Response> {
  const headers = new Headers(init.headers);
  if (options.authorization) {
    headers.set("Authorization", options.authorization);
  }
  const fetchImpl = options.fetchImpl ?? fetch;
  const response = await fetchImpl(url, { ...init, headers });
  if (!response.ok) {
    throw new CosFilesError(await errorMessage(response), response.status);
  }
  return response;
}

async function errorMessage(response: Response): Promise<string> {
  const text = await response.text();
  if (text.length === 0) {
    return response.statusText || "Request failed";
  }
  const head = text.slice(0, 200).toLowerCase();
  if (head.includes("<!doctype") || head.includes("<html")) {
    return "Cannot reach the device";
  }
  try {
    const body = JSON.parse(text) as { error?: unknown };
    if (typeof body.error === "string" && body.error.length > 0) {
      return body.error;
    }
  } catch {
    // The device returns plain text for authentication failures.
  }
  return text;
}

function isDirEntry(value: unknown): value is CosDirEntry {
  if (typeof value !== "object" || value === null) {
    return false;
  }
  const entry = value as Partial<CosDirEntry>;
  return (
    typeof entry.name === "string" &&
    isSafeEntryName(entry.name) &&
    typeof entry.size === "number" &&
    typeof entry.is_dir === "boolean"
  );
}

function compareEntries(a: CosDirEntry, b: CosDirEntry): number {
  if (a.is_dir !== b.is_dir) {
    return a.is_dir ? -1 : 1;
  }
  return a.name.localeCompare(b.name);
}
