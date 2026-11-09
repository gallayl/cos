import type ts from '@typescript/typescript6'

import { generateBerry } from './berry/code-generator'
import type { BerryGenerationResult } from './berry/diagnostics'

export type { BerryDiagnostic, BerryGenerationResult } from './berry/diagnostics'

/**
 * @brief Compile a TypeScript AST to Berry source.
 * @param ast Source file produced by {@link getAst}.
 */
export function astToBerry(ast: ts.SourceFile): BerryGenerationResult {
  return generateBerry(ast)
}
