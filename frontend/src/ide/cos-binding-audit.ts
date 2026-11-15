import type { Binding, NativeModuleInventory } from './cos-binding-inventory'

export function extractNativeModuleBindings(
  source: string,
  moduleName: string,
): { functions: string[]; constants: string[] } {
  const moduleMatch = source.match(
    new RegExp(
      String.raw`extern\s+"C"\s+const\s+bntvmodule_t\s+be_native_module_${moduleName}\s*=\s*\{[\s\S]*?"[^"]+"\s*,\s*(\w+)\s*,`,
      'm',
    ),
  )
  if (!moduleMatch) {
    throw new Error(`Could not find native module struct for ${moduleName}`)
  }
  const tableVar = moduleMatch[1]
  const tableMatch = source.match(
    new RegExp(
      String.raw`static\s+(?:const\s+)?bntvmodobj_t\s+(?:const\s+)?${tableVar}\s*\[\s*\]\s*=\s*\{([\s\S]*?)\n\};`,
      'm',
    ),
  )
  if (!tableMatch) {
    throw new Error(`Could not find native module registration table ${tableVar} for ${moduleName}`)
  }
  const table = tableMatch[1]
  const functions = [...table.matchAll(/be_native_module_function\("([^"]+)"/g)].map((m) => m[1])
  const constants = [...table.matchAll(/be_native_module_int\("([^"]+)"/g)].map((m) => m[1])
  return { functions, constants }
}

export function extractRegisteredGlobals(source: string): string[] {
  return [...source.matchAll(/be_regfunc\(s_vm,\s*"([^"]+)"/g)].map((m) => m[1])
}

export function bindingNames(bindings: readonly Binding[], kind: Binding['kind']): string[] {
  return bindings.filter((b) => b.kind === kind).map((b) => b.name)
}

export function assertSdkDeclaresBindings(
  module: NativeModuleInventory,
  declaration: string,
  options?: { ignore?: readonly string[] },
): void {
  const ignore = new Set(options?.ignore ?? [])
  for (const binding of module.bindings) {
    if (ignore.has(binding.name)) continue
    if (binding.kind === 'function') {
      const patterns = [`function ${binding.name}(`, `${binding.name}(`]
      if (!patterns.some((p) => declaration.includes(p))) {
        throw new Error(`${module.module} SDK declaration missing function ${binding.name}`)
      }
    } else {
      const patterns = [`const ${binding.name}:`, `readonly ${binding.name}:`, `${binding.name}:`]
      if (!patterns.some((p) => declaration.includes(p))) {
        throw new Error(`${module.module} SDK declaration missing const ${binding.name}`)
      }
    }
  }
}

export function assertModuleWrapper(specifier: string, wrapper: string): void {
  if (!wrapper.includes('typeof ')) {
    throw new Error(`@cos module wrapper for ${specifier} should reference a namespace/type`)
  }
  if (!wrapper.includes('export default')) {
    throw new Error(`@cos module wrapper for ${specifier} should export default`)
  }
}
