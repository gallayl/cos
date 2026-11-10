import type ts from '@typescript/typescript6'

import { generateBerry, type BerryCompileOptions } from './berry/code-generator'
import type { BerryGenerationResult } from './berry/diagnostics'

export type {
  BerryCompileMetadata,
  BerryCompileProfile,
  BerryDiagnostic,
  BerryDiagnosticSeverity,
  BerryGenerationResult,
  BerrySourceLocation,
  BerrySourceMapEntry,
} from './berry/diagnostics'
export { annotateBerryErrorText, findSourceLocation, formatMappedSource, mapBerryLineToSource } from './berry/source-map'
export type { BerryCompileOptions } from './berry/code-generator'

/**
 * @brief Compile a TypeScript AST to Berry source.
 * @param ast Source file produced by {@link getAst}.
 * @param options Compilation options. Defaults to strict COS entrypoint mode and dev profile.
 */
export function astToBerry(ast: ts.SourceFile, options?: BerryCompileOptions): BerryGenerationResult {
  return generateBerry(ast, options)
}
