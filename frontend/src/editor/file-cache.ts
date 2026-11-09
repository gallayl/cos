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
  dispose(): void
}

/** Build the device file caches. The service owns one instance for the app. */
export function createFileCaches(options: FileCacheOptions = {}): FileCacheService {
  const request: CosRequestOptions = { fetchImpl: options.fetchImpl }
  const directories = new Cache<CosDirEntry[], [string, string]>({
    load: (path, authorization) => listDirectory(path, withAuthorization(request, authorization)),
    staleTimeMs: options.directoryStaleTimeMs ?? directoryStaleMs,
    cacheTimeMs: options.directoryCacheTimeMs ?? directoryCacheMs,
  })
  const contents = new Cache<string, [string, string]>({
    load: (path, authorization) => readTextFile(path, withAuthorization(request, authorization)),
    staleTimeMs: options.fileStaleTimeMs ?? fileStaleMs,
    cacheTimeMs: options.fileCacheTimeMs ?? fileCacheMs,
  })

  return {
    directories,
    contents,
    refreshDirectory(path, authorization) {
      void directories.reload(path, authorization).catch(() => {
        // The directory cache entry records the failure for CacheView.
      })
    },
    rememberFile(path, authorization, text) {
      contents.setExplicitValue({
        loadArgs: [path, authorization],
        value: { status: 'loaded', value: text, updatedAt: new Date() },
      })
    },
    dispose() {
      /* eslint-disable furystack/prefer-using-wrapper -- These caches are created here but owned by the returned service; disposal is deferred to the caller/service lifetime, so a same-scope using() wrapper cannot express this ownership transfer. */
      directories[Symbol.dispose]()
      contents[Symbol.dispose]()
      /* eslint-enable furystack/prefer-using-wrapper */
    },
  }
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
