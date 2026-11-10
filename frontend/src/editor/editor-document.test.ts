import { describe, expect, it, vi } from 'vitest'

import {
  authorizationFromCredentials,
  berryPlan,
  canCreateBerry,
  canSaveDocument,
  confirmDiscard,
  createScratchDocument,
  documentAfterEdit,
  documentFromFile,
  fileEntryLabel,
  formatCompileSummary,
  formatDiagnostics,
  isDirty,
  lineCountLabel,
  locationText,
  markSaved,
  modelSyncChange,
  replaceText,
  savePlan,
  syncModels,
  type SyncableModel,
} from './editor-document'

describe('editor document', () => {
  it('tracks dirty text and the saved snapshot', () => {
    const opened = documentFromFile('device', '/flash/app.ts', 'export {}\n')
    expect(opened.language).toBe('typescript')
    expect(isDirty(opened)).toBe(false)

    expect(documentAfterEdit(opened, opened.text)).toBeNull()
    const edited = replaceText(opened, 'export { changed }\n')
    expect(documentAfterEdit(opened, edited.text)).toEqual(edited)
    expect(isDirty(edited)).toBe(true)
    expect(locationText(edited.path, true)).toBe('/flash/app.ts · modified')
    expect(isDirty(markSaved(edited))).toBe(false)
    expect(locationText('', false)).toBe('unsaved buffer')
    expect(lineCountLabel('a\nb')).toBe('2 lines')
  })

  it('asks before discarding a dirty buffer', () => {
    const confirm = vi.fn(() => false)
    expect(confirmDiscard(false, confirm)).toBe(true)
    expect(confirm).not.toHaveBeenCalled()
    expect(confirmDiscard(true, confirm)).toBe(false)
    expect(confirm).toHaveBeenCalledWith('Discard unsaved changes?')
  })

  it('plans save and Berry targets', () => {
    const scratch = createScratchDocument()
    expect(scratch.text).toBe('')
    expect(savePlan(scratch, false)).toEqual({ kind: 'save-as', suggestedName: 'script.ts' })
    expect(berryPlan(scratch)).toEqual({ kind: 'local', suggestedName: 'script.be' })
    expect(canCreateBerry('plaintext')).toBe(false)
    expect(canSaveDocument(scratch)).toBe(false)
    expect(canSaveDocument(null)).toBe(false)

    const device = documentFromFile('device', '/flash/app.ts', 'export {}\n')
    expect(canSaveDocument(device)).toBe(true)
    expect(savePlan(device, false)).toEqual({ kind: 'device', path: '/flash/app.ts' })
    expect(berryPlan(device)).toEqual({ kind: 'device', path: '/flash/app.be' })

    const local = documentFromFile('local', 'notes.txt', 'hi')
    expect(canSaveDocument(local)).toBe(false)
    expect(savePlan(local, true)).toEqual({ kind: 'handle' })
    expect(berryPlan(local)).toEqual({ kind: 'unsupported' })
    expect(fileEntryLabel('apps', true)).toBe('apps/')
    expect(fileEntryLabel('..', true)).toBe('..')
    expect(fileEntryLabel('app.ts', false)).toBe('app.ts')
  })

  it('formats diagnostics and builds a Basic header', () => {
    expect(formatDiagnostics([{ line: 2, column: 4, message: 'nope' }])).toBe('2:4 nope')
    expect(
      formatCompileSummary({
        ok: true,
        diagnostics: [],
        metadata: { profile: 'dev', mappings: [1, 2], notes: [] },
      }),
    ).toBe('Compiled · profile: dev · mappings: 2')
    expect(
      formatCompileSummary({
        ok: false,
        diagnostics: [{ severity: 'error' }, { severity: 'warning' }],
        metadata: { profile: 'prod', mappings: [], notes: [] },
      }),
    ).toBe('1 error(s) · profile: prod · mappings: 0')
    expect(authorizationFromCredentials('  ', 'x')).toEqual({ ok: false, message: 'Enter the device username' })
    expect(authorizationFromCredentials('cos', 'secret')).toEqual({
      ok: true,
      authorization: `Basic ${btoa('cos:secret')}`,
    })
    expect(authorizationFromCredentials('你', 'secret').ok).toBe(false)
  })

  it('syncs in-memory models and leaves other schemes alone', () => {
    const memory = fakeModel('inmemory', 'plaintext', 'old')
    const file = fakeModel('file', 'plaintext', 'old')
    syncModels([memory.model, file.model], 'new', 'typescript')

    expect(memory.setLanguage).toHaveBeenCalledWith('typescript')
    expect(memory.setValue).toHaveBeenCalledWith('new')
    expect(file.setLanguage).not.toHaveBeenCalled()
    expect(modelSyncChange({ scheme: 'inmemory', languageId: 'typescript', value: 'new' }, 'new', 'typescript')).toBe(
      null,
    )
  })
})

function fakeModel(
  scheme: string,
  languageId: string,
  value: string,
): {
  model: SyncableModel
  setLanguage: ReturnType<typeof vi.fn>
  setValue: ReturnType<typeof vi.fn>
} {
  const setLanguage = vi.fn()
  const setValue = vi.fn()
  return {
    setLanguage,
    setValue,
    model: {
      scheme,
      getLanguageId: () => languageId,
      getValue: () => value,
      setLanguage,
      setValue,
    },
  }
}
