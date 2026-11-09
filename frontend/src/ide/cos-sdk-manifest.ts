import berryBuiltinsTypes from '../cos-sdk/berry-builtins.d.ts?raw' with { type: 'text' }
import consoleTypes from '../cos-sdk/console.d.ts?raw' with { type: 'text' }
import filesystemTypes from '../cos-sdk/filesystem.d.ts?raw' with { type: 'text' }
import indexTypes from '../cos-sdk/index.d.ts?raw' with { type: 'text' }
import jsonTypes from '../cos-sdk/json.d.ts?raw' with { type: 'text' }
import languageTypes from '../cos-sdk/language.d.ts?raw' with { type: 'text' }
import networkTypes from '../cos-sdk/network.d.ts?raw' with { type: 'text' }
import uiTypes from '../cos-sdk/ui.d.ts?raw' with { type: 'text' }

import consoleModuleTypes from '../cos-sdk/modules/console.d.ts?raw' with { type: 'text' }
import fsModuleTypes from '../cos-sdk/modules/fs.d.ts?raw' with { type: 'text' }
import gcModuleTypes from '../cos-sdk/modules/gc.d.ts?raw' with { type: 'text' }
import globalModuleTypes from '../cos-sdk/modules/global.d.ts?raw' with { type: 'text' }
import introspectModuleTypes from '../cos-sdk/modules/introspect.d.ts?raw' with { type: 'text' }
import jsonModuleTypes from '../cos-sdk/modules/json.d.ts?raw' with { type: 'text' }
import mathModuleTypes from '../cos-sdk/modules/math.d.ts?raw' with { type: 'text' }
import netModuleTypes from '../cos-sdk/modules/net.d.ts?raw' with { type: 'text' }
import stringModuleTypes from '../cos-sdk/modules/string.d.ts?raw' with { type: 'text' }
import uiModuleTypes from '../cos-sdk/modules/ui.d.ts?raw' with { type: 'text' }

export type CosSdkLib = { content: string; filePath: string }
export type CosSdkModuleLib = CosSdkLib & { specifiers: readonly string[] }

export const sdkRoot = 'file:///cos-sdk'
export const modulesRoot = `${sdkRoot}/modules`

/** Declaration files passed to Monaco. Paths match the triple-slash references in `index.d.ts`. */
export const cosLibs: readonly CosSdkLib[] = [
  { content: languageTypes, filePath: `${sdkRoot}/language.d.ts` },
  { content: uiTypes, filePath: `${sdkRoot}/ui.d.ts` },
  { content: consoleTypes, filePath: `${sdkRoot}/console.d.ts` },
  { content: filesystemTypes, filePath: `${sdkRoot}/filesystem.d.ts` },
  { content: networkTypes, filePath: `${sdkRoot}/network.d.ts` },
  { content: jsonTypes, filePath: `${sdkRoot}/json.d.ts` },
  { content: berryBuiltinsTypes, filePath: `${sdkRoot}/berry-builtins.d.ts` },
  { content: indexTypes, filePath: `${sdkRoot}/index.d.ts` },
]

/**
 * `@cos/*` module declarations. `specifiers` include compatibility aliases
 * used by TS imports and by the Berry `@cos/*` runtime resolver.
 */
export const cosModuleLibs: readonly CosSdkModuleLib[] = [
  { content: consoleModuleTypes, filePath: `${modulesRoot}/console.d.ts`, specifiers: ['@cos/console'] },
  { content: fsModuleTypes, filePath: `${modulesRoot}/fs.d.ts`, specifiers: ['@cos/fs', '@cos/filesystem'] },
  { content: gcModuleTypes, filePath: `${modulesRoot}/gc.d.ts`, specifiers: ['@cos/gc'] },
  { content: globalModuleTypes, filePath: `${modulesRoot}/global.d.ts`, specifiers: ['@cos/global'] },
  { content: introspectModuleTypes, filePath: `${modulesRoot}/introspect.d.ts`, specifiers: ['@cos/introspect'] },
  { content: jsonModuleTypes, filePath: `${modulesRoot}/json.d.ts`, specifiers: ['@cos/json'] },
  { content: mathModuleTypes, filePath: `${modulesRoot}/math.d.ts`, specifiers: ['@cos/math'] },
  { content: netModuleTypes, filePath: `${modulesRoot}/net.d.ts`, specifiers: ['@cos/net', '@cos/network'] },
  { content: stringModuleTypes, filePath: `${modulesRoot}/string.d.ts`, specifiers: ['@cos/string'] },
  { content: uiModuleTypes, filePath: `${modulesRoot}/ui.d.ts`, specifiers: ['@cos/ui'] },
]

export const cosPaths = Object.fromEntries(
  cosModuleLibs.flatMap((lib) => lib.specifiers.map((specifier) => [specifier, [lib.filePath]])),
)
