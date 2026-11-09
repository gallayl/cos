import { describe, expect, it } from 'vitest'

import { collectRelativeImportSpecifiers } from './import-specifiers'

describe('collectRelativeImportSpecifiers', () => {
  it('collects relative type, value, and export-from specifiers', () => {
    const specifiers = collectRelativeImportSpecifiers(`
      import type { A } from "./a.ts";
      import { b } from "../b.ts";
      import "@scope/pkg";
      export { c } from "./c";
      export type { D } from "https://example.com/d.ts";
    `)

    expect(specifiers).toEqual(['./a.ts', '../b.ts', './c'])
  })

  it('ignores non-relative imports', () => {
    expect(collectRelativeImportSpecifiers(`import { ui } from "@cos/ui";`)).toEqual([])
  })
})
