import { Cache } from '@furystack/cache'
import { defineService } from '@furystack/inject'

import { listDirectory, readTextFile, type CosDirEntry, type CosRequestOptions } from '../ide/files/cos-files'

const directoryStaleMs = 10_000
const directoryCacheMs = 5 * 60_000
const fileStaleMs = 15_000
const fileCacheMs = 5 * 60_000

/** Options for {@link createFileCaches}. Tests pass a fake `fetch`. */
export interface FileCacheOptions {
  fetchImpl?: typeof fetch
  /** Entry becomes obsolete this many ms after a load. `0` disables the timer. */
  directoryStaleTimeMs?: number
  directoryCacheTimeMs?: number
  fileStaleTimeMs?: number
  fileCacheTimeMs?: number
}

/**
 * Cached device directory listings and file text.
 * Cache arguments are `[path, authorization]`. An empty authorization means no header.
 */
export interface FileCacheService {
  readonly directories: Cache<CosDirEntry[], [string, string]>
  readonly contents: Cache<string, [string, string]>
  /** Reload one directory listing. Failures stay on the cache entry. */
  refreshDirectory(path: string, authorization: string): void
  /** Store text that was just written so the next open can skip a read. */
  rememberFile(path: string, authorization: string, text: string): void
  /** Drop cached text for a file path. */
  invalidateFile(path: string, authorization: string): void
  /** Drop a cached directory listing. */
  invalidateDirectory(path: string, authorization: string): void
  /** Drop cached file text and directory listing for the same path. */
  invalidatePath(path: string, authorization: string): void
  /** Drop cached entries for a path and every descendant path. */
  invalidatePathTree(pathPrefix: string, authorization: string): void
  dispose(): void
}

/** Build the device file caches. The service owns one instance for the app. */
export function createFileCaches(options: FileCacheOptions = {}): FileCacheService {
  const request: CosRequestOptions = { fetchImpl: options.fetchImpl }
  const knownDirectories = new Map<string, Set<string>>()
  const knownFiles = new Map<string, Set<string>>()
  const directories = new Cache<CosDirEntry[], [string, string]>({
    load: async (path, authorization) => {
      rememberKnown(knownDirectories, authorization, path)
      return await listDirectory(path, withAuthorization(request, authorization))
    },
    staleTimeMs: options.directoryStaleTimeMs ?? directoryStaleMs,
    cacheTimeMs: options.directoryCacheTimeMs ?? directoryCacheMs,
  })
  const contents = new Cache<string, [string, string]>({
    load: async (path, authorization) => {
      rememberKnown(knownFiles, authorization, path)
      return await readTextFile(path, withAuthorization(request, authorization))
    },
    staleTimeMs: options.fileStaleTimeMs ?? fileStaleMs,
    cacheTimeMs: options.fileCacheTimeMs ?? fileCacheMs,
  })

  const invalidateFile = (path: string, authorization: string) => {
    contents.remove(path, authorization)
    forgetKnown(knownFiles, authorization, path)
  }

  const invalidateDirectory = (path: string, authorization: string) => {
    directories.remove(path, authorization)
    forgetKnown(knownDirectories, authorization, path)
  }

  return {
    directories,
    contents,
    refreshDirectory(path, authorization) {
      rememberKnown(knownDirectories, authorization, path)
      void directories.reload(path, authorization).catch(() => {
        // The directory cache entry records the failure for CacheView.
      })
    },
    rememberFile(path, authorization, text) {
      rememberKnown(knownFiles, authorization, path)
      contents.setExplicitValue({
        loadArgs: [path, authorization],
        value: { status: 'loaded', value: text, updatedAt: new Date() },
      })
    },
    invalidateFile,
    invalidateDirectory,
    invalidatePath(path, authorization) {
      invalidateFile(path, authorization)
      invalidateDirectory(path, authorization)
    },
    invalidatePathTree(pathPrefix, authorization) {
      removeMatching(knownFiles.get(authorization), pathPrefix, (path) => invalidateFile(path, authorization))
      removeMatching(knownDirectories.get(authorization), pathPrefix, (path) =>
        invalidateDirectory(path, authorization),
      )
    },
    dispose() {
      knownDirectories.clear()
      knownFiles.clear()
      /* eslint-disable furystack/prefer-using-wrapper -- These caches are created here but owned by the returned service; disposal is deferred to the caller/service lifetime, so a same-scope using() wrapper cannot express this ownership transfer. */
      directories[Symbol.dispose]()
      contents[Symbol.dispose]()
      /* eslint-enable furystack/prefer-using-wrapper */
    },
  }
}

function rememberKnown(store: Map<string, Set<string>>, authorization: string, path: string): void {
  const paths = store.get(authorization)
  if (paths) {
    paths.add(path)
    return
  }
  store.set(authorization, new Set([path]))
}

function forgetKnown(store: Map<string, Set<string>>, authorization: string, path: string): void {
  const paths = store.get(authorization)
  if (!paths) {
    return
  }
  paths.delete(path)
  if (paths.size === 0) {
    store.delete(authorization)
  }
}

function removeMatching(
  known: ReadonlySet<string> | undefined,
  pathPrefix: string,
  remove: (path: string) => void,
): void {
  if (!known) {
    return
  }
  for (const path of [...known]) {
    if (isPathOrDescendant(path, pathPrefix)) {
      remove(path)
    }
  }
}

function isPathOrDescendant(path: string, prefix: string): boolean {
  return path === prefix || path.startsWith(`${prefix}/`)
}

/** Device directory and file cache for the editor session. */
export const FileCache = defineService({
  name: 'cos/FileCache',
  lifetime: 'singleton',
  factory: ({ onDispose }) => {
    const cache = createFileCaches()
    onDispose(() => cache.dispose())
    return cache
  },
})

function withAuthorization(request: CosRequestOptions, authorization: string): CosRequestOptions {
  if (!authorization) {
    return request
  }
  return { ...request, authorization }
}
