import { javascriptDefaults, typescriptDefaults } from "monaco-editor/languages/features/typescript/register";

import { isTypeScriptPath } from "./files/paths";
import { modelFilePath } from "./workspace-paths";

export { modelFilePath, normalizeWorkspacePath } from "./workspace-paths";

const extraLibs = new Map<string, Array<{ dispose(): void }>>();

/**
 * Register a workspace TypeScript file with Monaco so relative `import type`
 * specifiers can resolve to it. Paths are normalized to `file://` URIs.
 * Re-registering the same path replaces the previous extra lib.
 */
export function registerWorkspaceTypeScriptFile(path: string, content: string): void {
  if (!isTypeScriptPath(path)) {
    return;
  }
  const filePath = modelFilePath(path);
  for (const lib of extraLibs.get(filePath) ?? []) {
    lib.dispose();
  }
  extraLibs.set(filePath, [
    typescriptDefaults.addExtraLib(content, filePath),
    javascriptDefaults.addExtraLib(content, filePath),
  ]);
}

