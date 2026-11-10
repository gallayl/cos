import {
  javascriptDefaults,
  ModuleKind,
  ScriptTarget,
  typescriptDefaults,
  type CompilerOptions,
} from 'monaco-editor/languages/features/typescript/register'

import { cosLibs, cosModuleLibs, cosPaths, sdkRoot } from './cos-sdk-manifest'

/**
 * TypeScript subset used by COS scripts.
 * `noLib` drops the DOM and ECMAScript default libraries, including `console.log`.
 * Bundler resolution with explicit `.ts` imports enables local type-only imports in the IDE.
 */
export const cosCompilerOptions: CompilerOptions = {
  allowNonTsExtensions: true,
  allowImportingTsExtensions: true,
  baseUrl: sdkRoot,
  target: ScriptTarget.ESNext,
  module: ModuleKind.ESNext,
  moduleResolution: 100 as unknown as CompilerOptions['moduleResolution'], // Bundler
  paths: cosPaths,
  noLib: true,
  strict: true,
}

export function registerCosSdk(): void {
  typescriptDefaults.setCompilerOptions(cosCompilerOptions)
  for (const lib of [...cosLibs, ...cosModuleLibs]) {
    typescriptDefaults.addExtraLib(lib.content, lib.filePath)
    javascriptDefaults.addExtraLib(lib.content, lib.filePath)
  }

  javascriptDefaults.setCompilerOptions({
    ...cosCompilerOptions,
    allowJs: true,
    checkJs: true,
  })
  javascriptDefaults.setDiagnosticsOptions({
    noSemanticValidation: false,
    noSyntaxValidation: false,
    onlyVisible: false,
  })
}

export { cosLibs, cosModuleLibs, cosPaths, sdkRoot }
