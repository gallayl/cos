import { describe, expect, it } from 'vitest'

import { modelFilePath, normalizeWorkspacePath } from './workspace-paths'

describe('workspace-paths', () => {
  it('normalizes slashes', () => {
    expect(normalizeWorkspacePath('\\flash\\app.ts')).toBe('/flash/app.ts')
  })

  it('builds file URIs for workspace paths', () => {
    expect(modelFilePath('/flash/app.ts')).toBe('file:///flash/app.ts')
    expect(modelFilePath('flash/app.ts')).toBe('file:///flash/app.ts')
    expect(modelFilePath('file:///flash/app.ts')).toBe('file:///flash/app.ts')
  })
})
