import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'

export function emitExportDeclaration(ctx: GenerationContext, node: ts.ExportDeclaration): void {
  if (node.isTypeOnly) return

  const specifierNode = node.moduleSpecifier
  const specifier = specifierNode && ts.isStringLiteral(specifierNode) ? specifierNode.text : undefined
  if (specifierNode && specifier === undefined) {
    ctx.report(node, 'Only string module specifiers are supported.')
    return
  }
  if (
    specifier !== undefined &&
    !specifier.startsWith('./') &&
    !specifier.startsWith('../') &&
    !specifier.startsWith('@cos/') &&
    !specifier.startsWith('@berry/')
  ) {
    ctx.report(node, 'Only relative re-exports and @cos/* module re-exports are supported.')
    return
  }

  const { exportClause } = node
  if (!exportClause) {
    ctx.report(node, 'export * re-exports are not supported yet.')
    return
  }

  if (ts.isNamespaceExport(exportClause)) {
    if (specifier === undefined) {
      ctx.report(node, 'Namespace exports require a module specifier.')
      return
    }
    const moduleTemp = ctx.nextTemp('__cos_re_')
    ctx.declareName(moduleTemp, node)
    ctx.writeLine(`var ${moduleTemp} = cos_import(${JSON.stringify(specifier)})`)
    ctx.noteNamedExport(exportClause.name.text, moduleTemp, node)
    return
  }

  let moduleTemp: string | undefined
  if (specifier !== undefined) {
    moduleTemp = ctx.nextTemp('__cos_re_')
    ctx.declareName(moduleTemp, node)
    ctx.writeLine(`var ${moduleTemp} = cos_import(${JSON.stringify(specifier)})`)
  }

  for (const element of exportClause.elements) {
    if (element.isTypeOnly) continue
    const exported = element.name.text
    if (moduleTemp) {
      const source = (element.propertyName ?? element.name).text
      const localTemp = ctx.nextTemp('__cos_export_')
      ctx.declareName(localTemp, element)
      ctx.writeLine(`var ${localTemp} = ${moduleTemp}.find(${JSON.stringify(source)})`)
      ctx.noteNamedExport(exported, localTemp, element)
    } else {
      const local = element.propertyName !== undefined ? element.propertyName.text : element.name.text
      if (exported === 'default') {
        ctx.noteDefaultExport(local, element)
      } else {
        ctx.noteNamedExport(exported, local, element)
      }
    }
  }
}

export function emitImportDeclaration(ctx: GenerationContext, node: ts.ImportDeclaration): void {
  if (node.importClause?.phaseModifier === ts.SyntaxKind.TypeKeyword) return
  if (!node.moduleSpecifier || !ts.isStringLiteral(node.moduleSpecifier)) {
    ctx.report(node, 'Only string module specifiers are supported.')
    return
  }

  const specifier = node.moduleSpecifier.text
  if (
    !specifier.startsWith('./') &&
    !specifier.startsWith('../') &&
    !specifier.startsWith('@cos/') &&
    !specifier.startsWith('@berry/')
  ) {
    ctx.report(node, 'Only relative imports and @cos/* module imports are supported.')
    return
  }

  const clause = node.importClause
  if (!clause) {
    ctx.report(node, 'Side-effect imports are not supported.')
    return
  }

  const { namedBindings } = clause
  const hasNamed = !!namedBindings && ts.isNamedImports(namedBindings) && namedBindings.elements.length > 0
  const hasNamespace = !!namedBindings && ts.isNamespaceImport(namedBindings)

  if (hasNamespace) {
    const local = namedBindings.name.text
    ctx.declareName(local, namedBindings.name)
    if (specifier.startsWith('@cos/') || specifier.startsWith('@berry/')) {
      const moduleTemp = ctx.nextTemp('__cos_mod_')
      ctx.declareName(moduleTemp, node)
      ctx.writeLine(`var ${moduleTemp} = cos_import(${JSON.stringify(specifier)})`)
      ctx.writeLine(`var ${local} = ${moduleTemp}.find("default")`)
    } else {
      ctx.writeLine(`var ${local} = cos_import(${JSON.stringify(specifier)})`)
    }
    if (clause.name) {
      ctx.report(clause.name, 'Default imports cannot be combined with namespace imports.')
    }
    return
  }

  if (!clause.name && !hasNamed) {
    if (namedBindings && ts.isNamedImports(namedBindings) && namedBindings.elements.length === 0) {
      ctx.report(node, 'Empty named imports are not supported.')
      return
    }
    ctx.report(node, 'Only default, named, and namespace imports are supported.')
    return
  }

  const moduleTemp = ctx.nextTemp('__cos_mod_')
  ctx.declareName(moduleTemp, node)
  ctx.writeLine(`var ${moduleTemp} = cos_import(${JSON.stringify(specifier)})`)
  const isCosModule = specifier.startsWith('@cos/') || specifier.startsWith('@berry/')

  if (clause.name) {
    ctx.declareName(clause.name.text, clause.name)
    ctx.writeLine(`var ${clause.name.text} = ${moduleTemp}.find("default")`)
  }

  if (namedBindings && ts.isNamedImports(namedBindings)) {
    let defaultTemp: string | undefined
    if (isCosModule && (clause.name == null || namedBindings.elements.length > 0)) {
      if (clause.name) {
        defaultTemp = clause.name.text
      } else {
        defaultTemp = ctx.nextTemp('__cos_def_')
        ctx.declareName(defaultTemp, node)
        ctx.writeLine(`var ${defaultTemp} = ${moduleTemp}.find("default")`)
      }
    }
    for (const element of namedBindings.elements) {
      if (element.isTypeOnly) continue
      const imported = (element.propertyName ?? element.name).text
      const local = element.name.text
      ctx.declareName(local, element.name)
      if (isCosModule) {
        ctx.writeLine(`var ${local} = ${defaultTemp}.${imported}`)
      } else {
        ctx.writeLine(`var ${local} = ${moduleTemp}.find(${JSON.stringify(imported)})`)
      }
    }
  }
}
