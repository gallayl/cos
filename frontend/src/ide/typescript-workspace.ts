import { Uri, editor } from 'monaco-editor'
import { javascriptDefaults, typescriptDefaults } from 'monaco-editor/languages/features/typescript/register'

import { isTypeScriptPath } from './files/paths'
import { modelFilePath, normalizeWorkspacePath } from './workspace-paths'

export { modelFilePath, normalizeWorkspacePath } from './workspace-paths'

const extraLibs = new Map<string, Array<{ dispose(): void }>>()
const registeredPaths = new Map<string, string>()

/**
 * Register a workspace TypeScript file with Monaco so relative `import type`
 * specifiers can resolve to it. Paths are normalized to `file://` URIs.
 * Re-registering the same path replaces the previous extra lib.
 */
export function registerWorkspaceTypeScriptFile(path: string, content: string): void {
  if (!isTypeScriptPath(path)) {
    return
  }
  const normalizedPath = normalizeWorkspacePath(path)
  const filePath = modelFilePath(normalizedPath)
  for (const lib of extraLibs.get(filePath) ?? []) {
    lib.dispose()
  }
  extraLibs.set(filePath, [
    typescriptDefaults.addExtraLib(content, filePath),
    javascriptDefaults.addExtraLib(content, filePath),
  ])
  registeredPaths.set(filePath, normalizedPath)
}

/** Remove a registered TypeScript extra lib and dispose its Monaco model, if any. */
export function unregisterWorkspaceTypeScriptFile(path: string): void {
  const filePath = modelFilePath(path)
  const libs = extraLibs.get(filePath)
  if (libs) {
    for (const lib of libs) {
      lib.dispose()
    }
    extraLibs.delete(filePath)
  }
  registeredPaths.delete(filePath)
  disposeWorkspaceModel(path)
}

/** Unregister every TypeScript file under `pathPrefix` and dispose matching Monaco models. */
export function unregisterWorkspaceTypeScriptTree(pathPrefix: string): void {
  const prefix = normalizeWorkspacePath(pathPrefix)
  for (const [filePath, path] of [...registeredPaths.entries()]) {
    if (isPathOrDescendant(path, prefix)) {
      unregisterWorkspaceTypeScriptFile(path)
      continue
    }
    void filePath
  }
}

/** Move/rename TypeScript registrations and Monaco models under a path prefix. */
export function moveWorkspaceTypeScriptTree(fromPath: string, toPath: string): void {
  const from = normalizeWorkspacePath(fromPath)
  const to = normalizeWorkspacePath(toPath)
  const moves = [...registeredPaths.values()]
    .filter((path) => isPathOrDescendant(path, from))
    .sort((left, right) => left.length - right.length)
  for (const path of moves) {
    const nextPath = movedWorkspacePath(path, from, to)
    if (!nextPath) {
      continue
    }
    const content = editor.getModel(Uri.parse(modelFilePath(path)))?.getValue()
    unregisterWorkspaceTypeScriptFile(path)
    if (content !== undefined) {
      registerWorkspaceTypeScriptFile(nextPath, content)
    }
  }
}

/** Dispose the Monaco text model for a workspace path, if it exists. */
export function disposeWorkspaceModel(path: string): void {
  editor.getModel(Uri.parse(modelFilePath(path)))?.dispose()
}

function movedWorkspacePath(path: string, from: string, to: string): string | null {
  if (path === from) {
    return to
  }
  if (path.startsWith(`${from}/`)) {
    return `${to}${path.slice(from.length)}`
  }
  return null
}

function isPathOrDescendant(path: string, prefix: string): boolean {
  return path === prefix || path.startsWith(`${prefix}/`)
}
