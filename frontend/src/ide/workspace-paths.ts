/** Normalize a workspace path to forward slashes. */
export function normalizeWorkspacePath(path: string): string {
  return path.replace(/\\/g, "/");
}

/** `file://` URI string used by Monaco models and TypeScript module resolution. */
export function modelFilePath(path: string): string {
  const normalized = normalizeWorkspacePath(path);
  if (normalized.startsWith("file://")) {
    return normalized;
  }
  return `file://${normalized.startsWith("/") ? normalized : `/${normalized}`}`;
}
