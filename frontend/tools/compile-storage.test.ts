import { mkdirSync, mkdtempSync, readFileSync, rmSync, utimesSync, writeFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join } from 'node:path'

import { describe, expect, it, vi } from 'vitest'

import {
  assertNoRelativeImportIssues,
  buildRelativeImportGraph,
  findRelativeImportCycle,
  main,
  verifyStorage,
  writeCompiledBerry,
} from './compile-storage'

describe('storage TypeScript scripts', () => {
  const report = verifyStorage()

  it('uses the COS SDK without JavaScript libraries', () => {
    expect(report.optionProblems).toEqual([])
    const messages = report.nonSdk.join('\n')
    expect(messages).toContain('console')
    expect(messages).toContain('setTimeout')
    expect(messages).toContain('Math')
  })

  it('typechecks every storage script', () => {
    expect(report.typecheck).toEqual([])
  })

  it('compiles every script to the device Berry program', () => {
    expect(main()).toBe(0)
    expect(report.scripts.map((script) => script.name)).toEqual([
      'apps/async_demo',
      'apps/breakout',
      'apps/tap_dash',
      'autoexec',
      'scripts/arg_test',
      'scripts/clear',
      'scripts/dir',
      'scripts/fetch_json',
      'scripts/files',
      'scripts/font_size',
      'scripts/hello',
      'scripts/host_info',
      'scripts/http_chat',
      'scripts/http_echo',
      'scripts/info',
      'scripts/light',
      'scripts/list_files',
      'scripts/ls',
      'scripts/mem',
      'scripts/memory',
      'scripts/nslookup',
      'scripts/ping',
      'scripts/pipe_stdin',
      'scripts/prompt',
      'scripts/set_base',
      'scripts/time',
      'scripts/uptime',
      'scripts/wget',
      'scripts/wifi',
    ])
    for (const script of report.scripts) {
      expect(script.diagnostics, script.name).toEqual([])
      expect(script.diff, script.name).toBe('')
    }
  })
})

describe('compile-storage incremental/dependency-aware (R23)', () => {
  function makeRoots() {
    const root = mkdtempSync(join(tmpdir(), 'cos-r23-'))
    const typescriptRoot = join(root, 'ts')
    const flashRoot = join(root, 'flash')
    mkdirSync(typescriptRoot, { recursive: true })
    mkdirSync(flashRoot, { recursive: true })
    return { root, typescriptRoot, flashRoot }
  }

  function writeTs(typescriptRoot: string, name: string, source: string) {
    const path = join(typescriptRoot, `${name}.ts`)
    mkdirSync(join(path, '..'), { recursive: true })
    writeFileSync(path, source)
    return path
  }

  const app = (body = 'return 0') =>
    `export default class App implements CosApplication {\n  run(_args: string[]): number {\n    ${body}\n  }\n}\n`

  it('discovers transitive relative deps and reports missing deps clearly', () => {
    const { root, typescriptRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'a', `import './b'\n${app()}`)
      writeTs(typescriptRoot, 'b', `import { c } from './sub/c'\n${app()}`)
      writeTs(typescriptRoot, 'sub/c', `${app()}`)
      const graph = buildRelativeImportGraph(typescriptRoot)
      expect(graph.deps.get('a')?.map((d) => d.resolved)).toEqual(['b'])
      expect(graph.deps.get('b')?.map((d) => d.resolved)).toEqual(['sub/c'])
      expect(graph.deps.get('sub/c')).toEqual([])
      expect(() => assertNoRelativeImportIssues(graph)).not.toThrow()

      writeTs(typescriptRoot, 'a', `import './missing'\n${app()}`)
      const missingGraph = buildRelativeImportGraph(typescriptRoot)
      expect(() => assertNoRelativeImportIssues(missingGraph)).toThrow(
        /a: missing\/unresolvable relative import '\.\/missing'/,
      )
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })

  it('detects relative import cycles with actionable deterministic error', () => {
    const { root, typescriptRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'x', `import './y'\n${app()}`)
      writeTs(typescriptRoot, 'y', `import './x'\n${app()}`)
      const cycleGraph = buildRelativeImportGraph(typescriptRoot)
      const cycle = findRelativeImportCycle(cycleGraph.deps, cycleGraph.names)
      expect(cycle).not.toBeNull()
      expect(() => assertNoRelativeImportIssues(cycleGraph)).toThrow(/relative import cycle:/)
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })

  it('write --write is per-file incremental and preserves unchanged/no-import behavior', () => {
    const { root, typescriptRoot, flashRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'plain', app())
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      const bePath = join(flashRoot, 'plain.be')
      const first = readFileSync(bePath, 'utf8')
      expect(first.length).toBeGreaterThan(0)
      const t1 = new Date('2026-01-01T00:00:00Z')
      const t2 = new Date('2026-01-02T00:00:00Z')
      utimesSync(join(typescriptRoot, 'plain.ts'), t1, t1)
      utimesSync(bePath, t2, t2) // up-to-date
      writeFileSync(bePath, 'SENTINEL\n')
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      expect(readFileSync(bePath, 'utf8')).toBe('SENTINEL\n') // skipped (unchanged/no-import behavior preserved)

      // stale output (older than ts) -> rewritten
      utimesSync(join(typescriptRoot, 'plain.ts'), t2, t2)
      utimesSync(bePath, t1, t1)
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      expect(readFileSync(bePath, 'utf8')).not.toBe('SENTINEL\n')

      // unchanged importer not forcibly recompiled when dep recompiles (per-file freshness)
      writeTs(typescriptRoot, 'dep', app())
      writeTs(typescriptRoot, 'importer', `import Dep from './dep'\n${app('var _d = Dep; return 0')}`)
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      const importerBe = join(flashRoot, 'importer.be')
      writeFileSync(importerBe, 'IMPORTER_SENTINEL\n')
      const t3 = new Date('2026-01-03T00:00:00Z')
      const t4 = new Date('2026-01-04T00:00:00Z')
      utimesSync(join(typescriptRoot, 'importer.ts'), t1, t1)
      utimesSync(importerBe, t2, t2) // importer up-to-date vs its own ts
      utimesSync(join(typescriptRoot, 'dep.ts'), t4, t4) // dep source newer
      utimesSync(join(flashRoot, 'dep.be'), t3, t3) // dep output stale -> will be rewritten
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      expect(readFileSync(importerBe, 'utf8')).toBe('IMPORTER_SENTINEL\n')
      expect(readFileSync(join(flashRoot, 'dep.be'), 'utf8')).not.toBe('')
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })

  it('recreates a missing .be output on --write', () => {
    const { root, typescriptRoot, flashRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'plain', app())
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      const bePath = join(flashRoot, 'plain.be')
      rmSync(bePath)
      expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(0)
      expect(readFileSync(bePath, 'utf8').length).toBeGreaterThan(0)
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })

  it('fails with actionable missing-import error on --write', () => {
    const { root, typescriptRoot, flashRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'a', `import './missing'\n${app()}`)
      const errSpy = vi.spyOn(console, 'error').mockImplementation(() => {})
      const logSpy = vi.spyOn(console, 'log').mockImplementation(() => {})
      try {
        expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(1)
        expect(errSpy.mock.calls.flat().join('\n')).toMatch(/missing\/unresolvable relative import '\.\/missing'/)
      } finally {
        errSpy.mockRestore()
        logSpy.mockRestore()
      }
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })

  it('fails with actionable cycle error on --write and does not recurse infinitely', () => {
    const { root, typescriptRoot, flashRoot } = makeRoots()
    try {
      writeTs(typescriptRoot, 'x', `import './y'\n${app()}`)
      writeTs(typescriptRoot, 'y', `import './x'\n${app()}`)
      const errSpy = vi.spyOn(console, 'error').mockImplementation(() => {})
      const logSpy = vi.spyOn(console, 'log').mockImplementation(() => {})
      try {
        expect(writeCompiledBerry({ typescriptRoot, flashRoot })).toBe(1)
        expect(errSpy.mock.calls.flat().join('\n')).toMatch(/relative import cycle:.*x.*y/)
      } finally {
        errSpy.mockRestore()
        logSpy.mockRestore()
      }
    } finally {
      rmSync(root, { recursive: true, force: true })
    }
  })
})
