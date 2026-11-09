import { basicAuthorization } from '../ide/files/cos-files'
import { berryPathFor, fileStem, languageForPath } from '../ide/files/paths'

/** Where the current buffer was loaded from. */
export type DocumentOrigin = 'none' | 'device' | 'local'

/** Text the editor is showing, plus enough metadata to save it. */
export interface EditorDocument {
  origin: DocumentOrigin
  path: string
  text: string
  savedText: string
  language: string
}

/** Scratch buffer shown at startup and after Reset. */
export const initialSource = `export default class Counter {
  private ticks = 0;

  setup(content: unknown, width: number, height: number): void {
    ui.timer(1000, () => this.tick());
  }

  private tick(): void {
    this.ticks += 1;
    log("ticks: " + this.ticks);
  }
}
`

/** A new unsaved TypeScript buffer. */
export function createScratchDocument(): EditorDocument {
  return {
    origin: 'none',
    path: '',
    text: initialSource,
    savedText: initialSource,
    language: 'typescript',
  }
}

/** Document opened from the device or the local disk. */
export function documentFromFile(origin: Exclude<DocumentOrigin, 'none'>, path: string, text: string): EditorDocument {
  return {
    origin,
    path,
    text,
    savedText: text,
    language: languageForPath(path),
  }
}

/** Replace the buffer text and keep the saved snapshot. */
export function replaceText(document: EditorDocument, text: string): EditorDocument {
  return { ...document, text }
}

/**
 * Apply text reported by the editor.
 * Returns null when the buffer already has that text, so a model refresh does not mark it modified.
 */
export function documentAfterEdit(document: EditorDocument, text: string): EditorDocument | null {
  if (text === document.text) {
    return null
  }
  return replaceText(document, text)
}

/** Treat the current text as saved. */
export function markSaved(document: EditorDocument): EditorDocument {
  return { ...document, savedText: document.text }
}

/** True when the buffer differs from the last saved snapshot. */
export function isDirty(document: EditorDocument): boolean {
  return document.text !== document.savedText
}

/** Ask before replacing a dirty buffer. Clean buffers proceed without asking. */
export function confirmDiscard(dirty: boolean, confirm: (message: string) => boolean): boolean {
  if (!dirty) {
    return true
  }
  return confirm('Discard unsaved changes?')
}

/** Status-bar label for the open path. */
export function locationText(path: string, dirty: boolean): string {
  const name = path || 'unsaved buffer'
  if (!dirty) {
    return name
  }
  return `${name} · modified`
}

/** Line count label for the status bar. */
export function lineCountLabel(text: string): string {
  return `${text.split('\n').length} lines`
}

/** Directory row label. Directories keep a trailing slash. */
export function fileEntryLabel(name: string, isDirectory: boolean): string {
  if (isDirectory && name !== '..') {
    return `${name}/`
  }
  return name
}

/** Join compiler diagnostics into the output panel. */
export function formatDiagnostics(
  diagnostics: ReadonlyArray<{ line: number; column: number; message: string }>,
): string {
  return diagnostics.map((diagnostic) => `${diagnostic.line}:${diagnostic.column} ${diagnostic.message}`).join('\n')
}

/** True when Create Berry can compile the open buffer. */
export function canCreateBerry(language: string): boolean {
  return language === 'typescript'
}

/** Where Save should write. */
export type SavePlan =
  { kind: 'device'; path: string } | { kind: 'handle' } | { kind: 'save-as'; suggestedName: string }

/** Choose a save target from the open document. */
export function savePlan(document: EditorDocument, hasLocalHandle: boolean): SavePlan {
  if (document.origin === 'device' && document.path) {
    return { kind: 'device', path: document.path }
  }
  if (document.origin === 'local' && hasLocalHandle) {
    return { kind: 'handle' }
  }
  return { kind: 'save-as', suggestedName: document.path || 'script.ts' }
}

/** Where Create Berry should write. */
export type BerryPlan =
  { kind: 'unsupported' } | { kind: 'device'; path: string } | { kind: 'local'; suggestedName: string }

/** Choose a Berry output target from the open document. */
export function berryPlan(document: EditorDocument): BerryPlan {
  if (!canCreateBerry(document.language)) {
    return { kind: 'unsupported' }
  }
  const sourceName = document.path || 'script.ts'
  if (document.origin === 'device') {
    const target = berryPathFor(document.path)
    if (target) {
      return { kind: 'device', path: target }
    }
  }
  return { kind: 'local', suggestedName: `${fileStem(sourceName)}.be` }
}

/** Failure message for the device sign-in form, or null when the header can be built. */
export function authorizationFromCredentials(
  username: string,
  password: string,
): { ok: true; authorization: string } | { ok: false; message: string } {
  if (username.trim() === '') {
    return { ok: false, message: 'Enter the device username' }
  }
  try {
    return { ok: true, authorization: basicAuthorization(username, password) }
  } catch {
    return { ok: false, message: 'Username and password must be ASCII' }
  }
}

/** Monaco model fields needed to decide whether a sync would change anything. */
export interface ModelSyncTarget {
  scheme: string
  languageId: string
  value: string
}

/** Fields a sync should write. Absent fields stay as they are. */
export interface ModelSyncChange {
  languageId?: string
  value?: string
}

/**
 * Change required to make an in-memory Monaco model match the open buffer.
 * Returns null for other URI schemes and for models that already match.
 */
export function modelSyncChange(model: ModelSyncTarget, text: string, languageId: string): ModelSyncChange | null {
  if (model.scheme !== 'inmemory') {
    return null
  }
  const change: ModelSyncChange = {}
  if (model.languageId !== languageId) {
    change.languageId = languageId
  }
  if (model.value !== text) {
    change.value = text
  }
  if (change.languageId === undefined && change.value === undefined) {
    return null
  }
  return change
}

/** Model operations used by {@link syncModels}. */
export interface SyncableModel {
  scheme: string
  getLanguageId(): string
  getValue(): string
  setLanguage(languageId: string): void
  setValue(value: string): void
}

/** Apply a buffer to every in-memory model that differs from it. */
export function syncModels(models: readonly SyncableModel[], text: string, languageId: string): void {
  for (const model of models) {
    const change = modelSyncChange(
      { scheme: model.scheme, languageId: model.getLanguageId(), value: model.getValue() },
      text,
      languageId,
    )
    if (!change) {
      continue
    }
    if (change.languageId) {
      model.setLanguage(change.languageId)
    }
    if (change.value !== undefined) {
      model.setValue(change.value)
    }
  }
}
