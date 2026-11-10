import { createComponent, Shade } from '@furystack/shades'
import { Button, NotyService, Typography, cssVariableTheme } from '@furystack/shades-common-components'

import { astToBerry } from '../ide/ast-to-berry'
import type { BerryCompileMetadata, BerryCompileProfile, BerryDiagnostic } from '../ide/ast-to-berry'
import { BerryCompileContext } from '../ide/berry-compile-context'
import { CosFilesError, describeCosError, writeTextFile } from '../ide/files/cos-files'
import { canOpenPath, languageForPath, parentPath } from '../ide/files/paths'
import { getAst } from '../ide/get-ast'
import '../ide/monaco-languages'
import '../ide/register-cos-sdk'
import { fetchReferencedWorkspaceTypes } from '../ide/import-resolution'
import { registerWorkspaceTypeScriptFile } from '../ide/typescript-workspace'
import { BerryErrorAnnotator } from './berry-error-annotator'
import { CompileOutput } from './compile-output'
import { DeviceAccess } from './device-access'
import {
  berryPlan,
  canSaveDocument,
  confirmDiscard,
  documentAfterEdit,
  documentFromFile,
  formatCompileSummary,
  formatDiagnostics,
  isDirty,
  lineCountLabel,
  locationText,
  savePlan,
  type EditorDocument,
} from './editor-document'
import { EditorStatusBar } from './editor-status-bar'
import { EditorToolbar } from './editor-toolbar'
import { FileBrowser } from './file-browser'
import { FileCache } from './file-cache'
import { movedPath } from './file-tree'
import { SourceEditor } from './source-editor'

const DEFAULT_COMPILE_PROFILE: BerryCompileProfile = 'dev'

/** COS TypeScript editor shell. */
export const EditorApp = Shade({
  customElementName: 'shades-editor-app',
  css: {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
    minHeight: '0',
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    '& .workspace': {
      flex: '1',
      minHeight: '0',
      display: 'flex',
    },
    '& .editor-column': {
      flex: '1',
      minWidth: '0',
      minHeight: '0',
      display: 'flex',
      flexDirection: 'column',
    },
    '& .empty-state': {
      flex: '1',
      minHeight: '0',
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      justifyContent: 'center',
      gap: cssVariableTheme.spacing.sm,
      color: cssVariableTheme.text.secondary,
      padding: cssVariableTheme.spacing.xl,
      textAlign: 'center',
    },
    '& .panels': {
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.paper,
      maxHeight: '18rem',
      overflow: 'auto',
    },
    '& .panel-actions': {
      display: 'flex',
      gap: cssVariableTheme.spacing.sm,
      padding: `${cssVariableTheme.spacing.xs} ${cssVariableTheme.spacing.md}`,
      alignItems: 'center',
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.paper,
      flexWrap: 'wrap',
    },
  },
  render: ({ useState, useDisposable, injector }) => {
    const [document, setDocument] = useState<EditorDocument | null>('document', null)
    const [output, setOutput] = useState('output', '')
    const [status, setStatus] = useState('status', 'No file open')
    const [revision, setRevision] = useState('revision', 0)
    const [berryDiagnostics, setBerryDiagnostics] = useState<readonly BerryDiagnostic[]>('berryDiagnostics', [])
    const [lastCompileMetadata, setLastCompileMetadata] = useState<BerryCompileMetadata | undefined>(
      'lastCompileMetadata',
      undefined,
    )
    const [showAnnotator, setShowAnnotator] = useState('showAnnotator', false)
    const live = useDisposable('live-document', () => ({
      document,
      [Symbol.dispose]() {},
    }))
    live.document = document

    const access = injector.get(DeviceAccess)
    const cache = injector.get(FileCache)
    const compileContext = injector.get(BerryCompileContext)
    const notyService = injector.get(NotyService)
    const dirty = document ? isDirty(document) : false

    const notify = (type: 'success' | 'error' | 'info' | 'warning', title: string, body?: string) => {
      notyService.emit('onNotyAdded', { type, title, body: body ?? '' })
    }

    useDisposable(
      'import-type-fetch',
      () => {
        if (!document || document.language !== 'typescript' || !document.path) {
          return { [Symbol.dispose]() {} }
        }
        const timeout = window.setTimeout(() => {
          void fetchReferencedWorkspaceTypes({
            documentPath: document.path,
            source: document.text,
            authorization: access.authorization.getValue(),
            fetcher: cache.contents,
            registerFile: registerWorkspaceTypeScriptFile,
          })
        }, 600)
        return {
          [Symbol.dispose]() {
            window.clearTimeout(timeout)
          },
        }
      },
      [document?.path, document?.text, document?.language],
    )

    useDisposable(
      'berry-live-diagnostics',
      () => {
        if (!document || document.language !== 'typescript') {
          setBerryDiagnostics([])
          return { [Symbol.dispose]() {} }
        }
        const timeout = window.setTimeout(() => {
          const current = live.document
          if (!current) return
          const result = astToBerry(getAst(current.text), { profile: DEFAULT_COMPILE_PROFILE })
          setBerryDiagnostics(result.diagnostics)
          setLastCompileMetadata(result.metadata)
          compileContext.setMetadata(result.metadata)
        }, 700)
        return {
          [Symbol.dispose]() {
            window.clearTimeout(timeout)
          },
        }
      },
      [document?.path, document?.text, document?.language, revision],
    )

    const discard = () => confirmDiscard(dirty, (message) => window.confirm(message))

    const noteFailure = (error: unknown, title = 'Operation failed') => {
      if (error instanceof CosFilesError && error.status === 401) {
        access.markUnauthorized()
      }
      const message = describeCosError(error)
      setStatus(message)
      notify('error', title, message)
    }

    const replaceDocument = (next: EditorDocument, nextStatus: string) => {
      setDocument(next)
      setOutput('')
      setBerryDiagnostics([])
      setLastCompileMetadata(undefined)
      compileContext.setMetadata(undefined)
      setRevision(revision + 1)
      setStatus(nextStatus)
    }

    const openDeviceFile = async (path: string) => {
      if (!canOpenPath(path)) {
        const msg = 'That file type cannot be opened as text'
        setStatus(msg)
        notify('warning', 'Cannot open file', msg)
        return
      }
      if (!discard()) {
        return
      }
      setStatus(`Opening ${path}`)
      try {
        const authorization = access.authorization.getValue()
        const text = await cache.contents.get(path, authorization)
        registerWorkspaceTypeScriptFile(path, text)
        void fetchReferencedWorkspaceTypes({
          documentPath: path,
          source: text,
          authorization,
          fetcher: cache.contents,
          registerFile: registerWorkspaceTypeScriptFile,
        })
        replaceDocument(documentFromFile('device', path, text), path)
      } catch (error) {
        noteFailure(error, 'Open failed')
      }
    }

    const save = async () => {
      const current = live.document
      if (!current) return
      const plan = savePlan(current, false)
      if (plan.kind !== 'device') {
        const msg = 'Only opened device files can be saved'
        setStatus(msg)
        notify('warning', 'Save unavailable', msg)
        return
      }
      const written = current.text
      const authorization = access.authorization.getValue()
      setStatus(`Saving ${plan.path}`)
      try {
        await writeTextFile(plan.path, written, { authorization })
        const latest = live.document
        if (latest && latest.path === plan.path) {
          setDocument({ ...latest, savedText: written })
        }
        registerWorkspaceTypeScriptFile(plan.path, written)
        cache.rememberFile(plan.path, authorization, written)
        const parent = parentPath(plan.path)
        if (parent) {
          cache.refreshDirectory(parent, authorization)
        }
        setStatus(`Saved ${plan.path}`)
        notify('success', 'Saved', plan.path)
      } catch (error) {
        noteFailure(error, 'Save failed')
      }
    }

    const preview = () => {
      const current = live.document
      if (!current) return undefined
      const result = astToBerry(getAst(current.text), { profile: DEFAULT_COMPILE_PROFILE })
      setBerryDiagnostics(result.diagnostics)
      setLastCompileMetadata(result.metadata)
      compileContext.setMetadata(result.metadata)
      if (result.ok) {
        setOutput(result.code)
        const summary = formatCompileSummary(result)
        setStatus(summary)
        notify('success', 'Preview ready', summary)
        return result
      }
      setOutput(formatDiagnostics(result.diagnostics))
      const summary = formatCompileSummary(result)
      setStatus(summary)
      notify('error', 'Preview failed', summary)
      return result
    }

    const saveBerry = async () => {
      const current = live.document
      if (!current) return
      const plan = berryPlan(current)
      if (plan.kind === 'unsupported') {
        const msg = 'Open a TypeScript file to save a Berry script'
        setStatus(msg)
        notify('warning', 'Cannot save Berry', msg)
        return
      }
      const result = preview()
      if (!result?.ok) return
      if (plan.kind !== 'device') {
        const msg = 'Berry output can only be saved for opened device TypeScript files'
        setStatus(msg)
        notify('warning', 'Save Berry unavailable', msg)
        return
      }
      try {
        const authorization = access.authorization.getValue()
        await writeTextFile(plan.path, result.code, { authorization })
        const parent = parentPath(plan.path)
        if (parent) {
          cache.refreshDirectory(parent, authorization)
        }
        setStatus(`Wrote ${plan.path}`)
        notify('success', 'Berry script saved', plan.path)
      } catch (error) {
        noteFailure(error, 'Save Berry failed')
      }
    }

    useDisposable(
      'save-shortcut',
      () => {
        const onKeyDown = (event: KeyboardEvent) => {
          if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 's') {
            event.preventDefault()
            if (canSaveDocument(live.document)) {
              void save()
            }
          }
        }
        window.addEventListener('keydown', onKeyDown)
        return {
          [Symbol.dispose]() {
            window.removeEventListener('keydown', onKeyDown)
          },
        }
      },
      [],
    )

    const hasMappings = (lastCompileMetadata?.mappings?.length ?? 0) > 0
    const isTs = document?.language === 'typescript'
    const canSave = canSaveDocument(document)
    const canPreview = isTs
    const canSaveBerry = isTs && document?.origin === 'device'

    return (
      <>
        <EditorToolbar
          canSave={canSave}
          canPreview={canPreview}
          canSaveBerry={!!canSaveBerry}
          onSave={() => void save()}
          onPreview={preview}
          onSaveBerry={() => void saveBerry()}
        />
        <div className="workspace">
          <FileBrowser
            activePath={document?.path ?? ''}
            onOpenFile={(path) => void openDeviceFile(path)}
            onActivity={setStatus}
            onMoved={(from, to) => {
              const current = live.document
              if (!current) return
              const next = movedPath(current.path, from, to)
              if (!next) return
              cache.rememberFile(next, access.authorization.getValue(), current.text)
              setDocument({ ...current, path: next, language: languageForPath(next) })
              setStatus(next)
            }}
          />
          <div className="editor-column">
            {document ? (
              <>
                <SourceEditor
                  value={document.text}
                  language={document.language}
                  path={document.path}
                  revision={revision}
                  berryDiagnostics={berryDiagnostics}
                  onValueChange={(text) => {
                    const current = live.document
                    if (!current) return
                    const next = documentAfterEdit(current, text)
                    if (next) {
                      setDocument(next)
                    }
                  }}
                />
                {output || (isTs && showAnnotator) ? (
                  <div className="panels">
                    {output ? <CompileOutput text={output} /> : null}
                    {isTs && showAnnotator ? <BerryErrorAnnotator metadata={lastCompileMetadata} /> : null}
                  </div>
                ) : null}
                <div className="panel-actions">
                  {output ? (
                    <Button type="button" size="small" variant="text" onclick={() => setOutput('')}>
                      Hide output
                    </Button>
                  ) : null}
                  {isTs ? (
                    <Button type="button" size="small" variant="text" onclick={() => setShowAnnotator(!showAnnotator)}>
                      {showAnnotator ? 'Hide Berry mapping' : hasMappings ? 'Show Berry mapping' : 'Berry mapping'}
                    </Button>
                  ) : null}
                  {hasMappings ? (
                    <span
                      style={{ fontSize: cssVariableTheme.typography.fontSize.xs, color: cssVariableTheme.text.secondary }}
                    >
                      {lastCompileMetadata?.mappings?.length} mapping(s)
                    </span>
                  ) : null}
                </div>
              </>
            ) : (
              <div className="empty-state">
                <Typography variant="h6" color="textSecondary">
                  No file open
                </Typography>
                <Typography variant="body2" color="textSecondary">
                  Select a device file from the browser to open it. Upload files or folders by dragging them onto a
                  specific folder in the sidebar.
                </Typography>
              </div>
            )}
          </div>
        </div>
        <EditorStatusBar
          status={status}
          location={document ? locationText(document.path, dirty) : 'No file open'}
          lines={document ? lineCountLabel(document.text) : ''}
        />
      </>
    )
  },
})
