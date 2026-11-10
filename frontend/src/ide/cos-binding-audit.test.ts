/* eslint-disable @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access, @typescript-eslint/no-unsafe-return -- Node-only test helpers; frontend tsconfig omits @types/node. */
// @ts-expect-error - Vitest runs in Node; frontend tsc omits @types/node.
import { readFileSync } from 'node:fs'
// @ts-expect-error - Vitest runs in Node; frontend tsc omits @types/node.
import path from 'node:path'
// @ts-expect-error - Vitest runs in Node; frontend tsc omits @types/node.
import { fileURLToPath } from 'node:url'
import { describe, expect, it } from 'vitest'

import {
  berryBuiltinModuleSpecifiers,
  cosGlobalFunctions,
  cosNativeModules,
  publicBindings,
} from './cos-binding-inventory'
import {
  assertModuleWrapper,
  assertSdkDeclaresBindings,
  bindingNames,
  extractNativeModuleBindings,
  extractRegisteredGlobals,
} from './cos-binding-audit'
import { cosLibs, cosModuleLibs, cosPaths } from './cos-sdk-manifest'

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../..')

function repoFile(relativePath: string): string {
  return path.join(repoRoot, relativePath)
}

function readRepoFile(relativePath: string): string {
  return readFileSync(repoFile(relativePath), 'utf8')
}

function expectRepoFile(relativePath: string): void {
  expect(() => readRepoFile(relativePath), `missing ${relativePath}`).not.toThrow()
}

describe('R11 COS binding audit', () => {
  it('inventory C++ sources exist', () => {
    for (const module of cosNativeModules) expectRepoFile(module.source)
    for (const global of cosGlobalFunctions) expectRepoFile(global.source)
  })

  it('native module inventory matches C++ registration tables', () => {
    for (const module of cosNativeModules) {
      const source = readRepoFile(module.source)
      const actual = extractNativeModuleBindings(source, module.module)
      expect(actual.functions.sort(), `${module.module} functions`).toEqual(
        bindingNames(module.bindings, 'function').sort(),
      )
      expect(actual.constants.sort(), `${module.module} constants`).toEqual(
        bindingNames(module.bindings, 'const').sort(),
      )
    }
  })

  it('public COS globals are registered in berry_runtime.cpp', () => {
    const source = readRepoFile('components/berry_lang/src/berry_runtime.cpp')
    const actual = new Set(extractRegisteredGlobals(source))
    for (const global of cosGlobalFunctions) {
      expect(actual.has(global.name), `missing be_regfunc ${global.name}`).toBe(true)
    }
  })

  it('global SDK declarations cover COS native namespaces and globals', () => {
    const all = cosLibs.map((lib) => lib.content).join('\n')
    for (const module of cosNativeModules) {
      if (!module.globalNamespace) continue
      expect(all, `missing namespace ${module.globalNamespace}`).toContain(`namespace ${module.globalNamespace}`)
      const globalFile =
        module.globalNamespace === 'fs'
          ? 'filesystem'
          : module.globalNamespace === 'net'
            ? 'network'
            : module.globalNamespace
      const lib = cosLibs.find((entry) => entry.filePath.endsWith(`/${globalFile}.d.ts`))
      expect(lib, `missing global declaration file for ${module.module}`).toBeDefined()
      assertSdkDeclaresBindings(module, lib!.content, {
        ignore:
          publicBindings(module).length === module.bindings.length
            ? []
            : module.bindings.filter((b) => b.internal).map((b) => b.name),
      })
    }
    for (const global of cosGlobalFunctions) {
      expect(all, `missing global ${global.name}`).toContain(global.declarationPattern)
    }
  })

  it('manifest registers @cos and @berry specifiers for COS native modules', () => {
    for (const module of cosNativeModules) {
      for (const specifier of module.specifiers) {
        expect(cosPaths[specifier], `missing manifest mapping for ${specifier}`).toBeDefined()
        const filePath = cosPaths[specifier][0]
        expect(filePath).toContain(`/modules/`)
        const moduleLib = cosModuleLibs.find((lib) => lib.filePath === filePath)
        expect(moduleLib, `missing module lib for ${specifier}`).toBeDefined()
        expect(moduleLib!.specifiers).toContain(specifier)
        assertModuleWrapper(specifier, moduleLib!.content)
      }
    }
  })

  it('manifest registers @cos and @berry specifiers for Berry built-in modules', () => {
    for (const [name, specifiers] of Object.entries(berryBuiltinModuleSpecifiers)) {
      for (const specifier of specifiers) {
        expect(cosPaths[specifier], `missing ${name} mapping for ${specifier}`).toBeDefined()
      }
    }
  })

  it('keeps legacy filesystem/network aliases', () => {
    expect(cosPaths['@cos/fs']).toEqual(cosPaths['@cos/filesystem'])
    expect(cosPaths['@cos/net']).toEqual(cosPaths['@cos/network'])
    expect(cosPaths['@berry/fs']).toEqual(cosPaths['@berry/filesystem'])
    expect(cosPaths['@berry/net']).toEqual(cosPaths['@berry/network'])
  })
})
