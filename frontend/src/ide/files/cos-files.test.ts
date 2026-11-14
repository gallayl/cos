import { describe, expect, it, vi } from 'vitest'

import {
  CosFilesError,
  basicAuthorization,
  createDirectory,
  deleteEntry,
  describeCosError,
  encodePathQuery,
  listDirectory,
  readTextFile,
  renameEntry,
  uploadFile,
  writeTextFile,
} from './cos-files'

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { 'Content-Type': 'application/json' },
  })
}

describe('cos files', () => {
  it('lists a directory with directories first', async () => {
    const fetchImpl = vi.fn(async (_url: string, _init?: RequestInit) =>
      jsonResponse([
        { name: 'b.txt', size: 2, is_dir: false, mtime: 20, ctime: 10 },
        { name: 'apps', size: 0, is_dir: true, mtime: 30, ctime: 15 },
        { name: 'a.txt', size: 1, is_dir: false, mtime: 10, ctime: 5 },
        { name: '../secret', size: 1, is_dir: false, mtime: 1, ctime: 1 },
      ]),
    )

    const entries = await listDirectory('/flash', {
      fetchImpl: fetchImpl as typeof fetch,
      authorization: 'Basic abc',
    })

    expect(fetchImpl).toHaveBeenCalledWith('/api/files?path=/flash', expect.anything())
    const init = fetchImpl.mock.calls[0]?.[1]
    expect(new Headers(init?.headers).get('Authorization')).toBe('Basic abc')
    expect(entries.map((entry) => entry.name)).toEqual(['apps', 'a.txt', 'b.txt'])
  })

  it('reads file text', async () => {
    const fetchImpl = vi.fn(async () => new Response('export default class App {}\n', { status: 200 }))
    const text = await readTextFile('/flash/app.ts', { fetchImpl })
    expect(text).toContain('class App')
    expect(fetchImpl).toHaveBeenCalledWith('/api/file?path=/flash/app.ts', expect.anything())
  })

  it('uploads text with the path field before the file', async () => {
    const fetchImpl = vi.fn(async (_url: string, init?: RequestInit) => {
      const body = init?.body as FormData
      const names = [...body.keys()]
      expect(names).toEqual(['path', 'file'])
      expect(body.get('path')).toBe('/flash/app.ts')
      const file = body.get('file')
      expect(file).toBeInstanceOf(Blob)
      expect(await (file as Blob).text()).toBe('let ticks = 0\n')
      return jsonResponse({ status: 'ok' })
    })

    await writeTextFile('/flash/app.ts', 'let ticks = 0\n', { fetchImpl: fetchImpl as typeof fetch })
  })

  it('creates, renames, deletes, and uploads through the filesystem API', async () => {
    const calls: Array<{ url: string; method: string }> = []
    const fetchImpl = vi.fn(async (url: string, init?: RequestInit) => {
      calls.push({ url, method: init?.method ?? 'GET' })
      if (url.startsWith('/api/upload')) {
        const body = init?.body as FormData
        expect(body.get('path')).toBe('/flash/apps/icon.png')
        expect(body.get('file')).toBeInstanceOf(Blob)
      }
      return jsonResponse({ status: 'ok' })
    })
    const options = { fetchImpl: fetchImpl as typeof fetch, authorization: 'Basic abc' }

    await createDirectory('/flash/apps', options)
    await renameEntry('/flash/apps', '/flash/widgets', options)
    await deleteEntry('/flash/widgets/old.ts', options)
    await uploadFile('/flash/apps/icon.png', new Blob([Uint8Array.from([1, 2])]), options)

    expect(calls).toEqual([
      { url: '/api/mkdir?path=/flash/apps', method: 'POST' },
      { url: '/api/rename?from=/flash/apps&to=/flash/widgets', method: 'POST' },
      { url: '/api/file?path=/flash/widgets/old.ts', method: 'DELETE' },
      { url: '/api/upload', method: 'POST' },
    ])
    expect(new Headers(fetchImpl.mock.calls[0]?.[1]?.headers).get('Authorization')).toBe('Basic abc')
  })

  it('rejects a file list that is not JSON', async () => {
    const fetchImpl = vi.fn(async () => new Response('<!doctype html>', { status: 200 }))
    await expect(listDirectory('/flash', { fetchImpl })).rejects.toMatchObject({
      message: 'Device file list is unavailable',
    })
  })

  it('reports device errors and authentication failures', async () => {
    const fetchImpl = vi.fn(async () => new Response('Unauthorized', { status: 401 }))
    await expect(readTextFile('/flash/app.ts', { fetchImpl })).rejects.toMatchObject({
      message: 'Unauthorized',
      status: 401,
    })

    const described = describeCosError(new CosFilesError('Not found', 404))
    expect(described).toBe('Not found')
    expect(describeCosError(new TypeError('Failed to fetch'))).toBe('Cannot reach the device')
    expect(describeCosError('unexpected')).toBe('Request failed')
    expect(basicAuthorization('cos', 'secret')).toBe(`Basic ${btoa('cos:secret')}`)
  })

  it('encodes path queries without mangling slashes', () => {
    expect(encodePathQuery('/flash/apps/My File.ts')).toBe('/flash/apps/My%20File.ts')
    expect(encodePathQuery('/flash/café.txt')).toBe('/flash/caf%C3%A9.txt')
  })

  it('surfaces JSON API error messages and predictable fallback bodies', async () => {
    await expect(
      readTextFile('/missing.ts', {
        fetchImpl: vi.fn(async () => jsonResponse({ error: 'Not found' }, 404)),
      }),
    ).rejects.toMatchObject({ message: 'Not found', status: 404 })

    await expect(
      createDirectory('/flash/apps', {
        fetchImpl: vi.fn(async () => new Response('<!doctype html><p>proxy</p>', { status: 502 })),
      }),
    ).rejects.toMatchObject({ message: 'Cannot reach the device', status: 502 })

    await expect(
      deleteEntry('/flash/apps', {
        fetchImpl: vi.fn(async () => new Response('', { status: 500, statusText: 'Internal Server Error' })),
      }),
    ).rejects.toMatchObject({ message: 'Internal Server Error', status: 500 })
  })

  it('propagates auth and mutation failures predictably', async () => {
    const fetchImpl = vi.fn(async (_url: string | URL | Request, _init?: RequestInit) =>
      jsonResponse({ error: 'Forbidden' }, 403),
    )
    const options = { fetchImpl: fetchImpl as typeof fetch, authorization: 'Basic abc' }

    await expect(renameEntry('/flash/a.ts', '/flash/b.ts', options)).rejects.toMatchObject({
      message: 'Forbidden',
      status: 403,
    })
    await expect(uploadFile('/flash/a.ts', new Blob(['x']), options)).rejects.toMatchObject({
      message: 'Forbidden',
      status: 403,
    })
    expect(new Headers(fetchImpl.mock.calls[0]?.[1]?.headers).get('Authorization')).toBe('Basic abc')
  })
})
