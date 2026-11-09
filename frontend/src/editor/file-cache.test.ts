import { createInjector } from '@furystack/inject'
import { describe, expect, it, vi } from 'vitest'

import { FileCache, createFileCaches } from './file-cache'

describe('file cache', () => {
  it('reuses a directory listing until it is refreshed', async () => {
    const fetchImpl = vi.fn(async (_url: string, _init?: RequestInit) =>
      json([{ name: 'app.ts', size: 1, is_dir: false }]),
    )
    const cache = createFileCaches({
      fetchImpl: fetchImpl as typeof fetch,
      directoryStaleTimeMs: 0,
      directoryCacheTimeMs: 0,
      fileStaleTimeMs: 0,
      fileCacheTimeMs: 0,
    })

    const first = await cache.directories.get('/flash', 'Basic abc')
    const second = await cache.directories.get('/flash', 'Basic abc')
    cache.refreshDirectory('/flash', 'Basic abc')
    await vi.waitFor(() => expect(fetchImpl).toHaveBeenCalledTimes(2))

    expect(first).toEqual(second)
    expect(fetchImpl.mock.calls[0]?.[0]).toBe('/api/files?path=/flash')
    const init = fetchImpl.mock.calls[0]?.[1]
    expect(new Headers(init?.headers).get('Authorization')).toBe('Basic abc')
    cache.dispose()
  })

  it('remembers written file text without reading it back', async () => {
    const fetchImpl = vi.fn(async () => new Response('from-device', { status: 200 }))
    const cache = createFileCaches({
      fetchImpl,
      directoryStaleTimeMs: 0,
      directoryCacheTimeMs: 0,
      fileStaleTimeMs: 0,
      fileCacheTimeMs: 0,
    })

    cache.rememberFile('/flash/app.ts', '', 'let ticks = 0\n')
    const text = await cache.contents.get('/flash/app.ts', '')
    expect(text).toBe('let ticks = 0\n')
    expect(fetchImpl).not.toHaveBeenCalled()
    cache.dispose()
  })

  it('is registered as an injector singleton', async () => {
    const injector = createInjector()
    const cache = injector.get(FileCache)
    expect(cache.directories).toBe(injector.get(FileCache).directories)
    await injector[Symbol.asyncDispose]()
  })
})

function json(body: unknown): Response {
  return new Response(JSON.stringify(body), {
    status: 200,
    headers: { 'Content-Type': 'application/json' },
  })
}
