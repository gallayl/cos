import { javascriptDefaults, typescriptDefaults } from "monaco-editor/languages/features/typescript/register";

import { isTypeScriptPath } from "./files/paths";

/**
 * Register a workspace TypeScript file with Monaco so relative `import type`
 * specifiers can resolve to it. Paths are normalized to `file://` URIs.
 */
export function registerWorkspaceTypeScriptFile(path: string, content: string): void {
  if (!isTypeScriptPath(path)) {
    return;
  }
  const filePath = modelFilePath(path);
  typescriptDefaults.addExtraLib(content, filePath);
  javascriptDefaults.addExtraLib(content, filePath);
}

/** `file://` URI string used by Monaco models and TypeScript module resolution. */
export function modelFilePath(path: string): string {
  const normalized = path.replace(/\\/g, "/");
  if (normalized.startsWith("file://")) {
    return normalized;
  }
  return `file://${normalized.startsWith("/") ? normalized : `/${normalized}`}`;
}
