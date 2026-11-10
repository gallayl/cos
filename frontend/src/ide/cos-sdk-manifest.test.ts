import { describe, expect, it } from 'vitest'

import { cosLibs, cosModuleLibs, cosPaths, modulesRoot, sdkRoot } from './cos-sdk-manifest'

const moduleSources = import.meta.glob<string>('../cos-sdk/modules/*.d.ts', {
  eager: true,
  query: '?raw',
  import: 'default',
})

describe('COS SDK manifest', () => {
  it('registers every global SDK declaration file with a stable virtual path', () => {
    expect(cosLibs.map((lib) => lib.filePath)).toEqual([
      `${sdkRoot}/language.d.ts`,
      `${sdkRoot}/ui.d.ts`,
      `${sdkRoot}/console.d.ts`,
      `${sdkRoot}/filesystem.d.ts`,
      `${sdkRoot}/network.d.ts`,
      `${sdkRoot}/json.d.ts`,
      `${sdkRoot}/berry-builtins.d.ts`,
      `${sdkRoot}/index.d.ts`,
    ])
    expect(cosLibs.every((lib) => lib.content.length > 0)).toBe(true)
  })

  it('covers every @cos module declaration file and maps all specifiers', () => {
    const discovered = Object.keys(moduleSources)
      .map((key) => key.replace(/^.*\/modules\//, ''))
      .sort()
    const registered = cosModuleLibs.map((lib) => lib.filePath.replace(`${modulesRoot}/`, '')).sort()

    expect(registered).toEqual(discovered)

    for (const lib of cosModuleLibs) {
      expect(lib.content.length, `${lib.filePath} should not be empty`).toBeGreaterThan(0)
      expect(lib.specifiers.length, `${lib.filePath} should register at least one specifier`).toBeGreaterThan(0)
      for (const specifier of lib.specifiers) {
        expect(cosPaths[specifier], `missing path mapping for ${specifier}`).toEqual([lib.filePath])
      }
    }
  })

  it('keeps compatibility aliases for filesystem and network modules', () => {
    expect(cosPaths['@cos/fs']).toEqual([`${modulesRoot}/fs.d.ts`])
    expect(cosPaths['@cos/filesystem']).toEqual([`${modulesRoot}/fs.d.ts`])
    expect(cosPaths['@berry/fs']).toEqual([`${modulesRoot}/fs.d.ts`])
    expect(cosPaths['@berry/filesystem']).toEqual([`${modulesRoot}/fs.d.ts`])
    expect(cosPaths['@cos/net']).toEqual([`${modulesRoot}/net.d.ts`])
    expect(cosPaths['@cos/network']).toEqual([`${modulesRoot}/net.d.ts`])
    expect(cosPaths['@berry/net']).toEqual([`${modulesRoot}/net.d.ts`])
    expect(cosPaths['@berry/network']).toEqual([`${modulesRoot}/net.d.ts`])
  })
})
