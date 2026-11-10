import { extensionOf, isTypeScriptPath, parentPath } from './files/paths'
import { collectRelativeImportSpecifiers } from './import-specifiers'

export interface WorkspaceImportFetcher {
  get(path: string, authorization: string): Promise<string>
}

export interface FetchReferencedWorkspaceTypesOptions {
  documentPath: string
  source: string
  authorization: string
  fetcher: WorkspaceImportFetcher
  registerFile(path: string, content: string): void
}

/**
 * Resolve and fetch relative TypeScript imports referenced by `source`.
 * Fetched files are passed to `registerFile` for Monaco IntelliSense. Failures
 * are ignored because missing files may be created later or be unavailable.
 */
export async function fetchReferencedWorkspaceTypes(options: FetchReferencedWorkspaceTypesOptions): Promise<void> {
  const directory = parentPath(options.documentPath)
  if (!directory) {
    return
  }

  const seen = new Set<string>()
  for (const specifier of collectRelativeImportSpecifiers(options.source)) {
    const target = resolveImportPath(directory, specifier)
    if (!target || seen.has(target) || !isDevicePath(target)) {
      continue
    }
    seen.add(target)
    try {
      const text = await options.fetcher.get(target, options.authorization)
      options.registerFile(target, text)
    } catch {
      // Best-effort: the file may not exist yet or the device may be unavailable.
    }
  }
}

/** Resolve a relative import specifier from a device directory to a TS path, or null when unsupported. */
export function resolveImportPath(directory: string, specifier: string): string | null {
  const base = normalizePathSegments(`${directory}/${specifier}`)
  if (isTypeScriptPath(base)) {
    return base
  }
  if (extensionOf(base) !== '') {
    return null
  }
  return `${base}.ts`
}

function normalizePathSegments(path: string): string {
  const parts = path.replace(/\\/g, '/').split('/')
  const stack: string[] = []
  for (const part of parts) {
    if (part === '' || part === '.') {
      if (stack.length === 0 && part === '') stack.push('')
      continue
    }
    if (part === '..') {
      if (stack.length > 1) stack.pop()
      continue
    }
    stack.push(part)
  }
  const joined = stack.join('/')
  return joined.startsWith('/') ? joined : `/${joined}`
}

function isDevicePath(path: string): boolean {
  return path.startsWith('/flash/') || path === '/flash' || path.startsWith('/sdcard/') || path === '/sdcard'
}
