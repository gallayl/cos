import { createComponent, Shade } from '@furystack/shades'
import { cssVariableTheme } from '@furystack/shades-common-components'
import { MarkerSeverity, Uri, editor } from 'monaco-editor'

import type { BerryDiagnostic } from '../ide/ast-to-berry'
import { modelFilePath, registerWorkspaceTypeScriptFile } from '../ide/typescript-workspace'

export interface SourceEditorProps {
  value: string
  language: string
  /** Device/workspace path used for TypeScript module resolution in Monaco. */
  path: string
  /** Bumped when the buffer is replaced from outside Monaco. */
  revision: number
  onValueChange: (value: string) => void
  /** Berry compiler diagnostics shown as Monaco markers (TS files). */
  berryDiagnostics?: readonly BerryDiagnostic[]
}

/**
 * Monaco editor backed by a path-addressed model so relative TypeScript imports
 * (for example `import type { T } from "./types.ts"`) resolve in the IDE.
 */
export const SourceEditor = Shade<SourceEditorProps>({
  customElementName: 'cos-source-editor',
  css: {
    display: 'block',
    flex: '1',
    minHeight: '0',
    minWidth: '0',
    padding: cssVariableTheme.spacing.md,
    '& .editor-container': {
      width: '100%',
      height: '100%',
      minHeight: '0',
      minWidth: '0',
    },
  },
  render: ({ props, useDisposable, useRef }) => {
    const container = useRef<HTMLDivElement>('editor-container')
    const modelUri = modelUriForPath(props.path, props.language)

    const onChange = useDisposable('on-change', () => ({
      current: props.onValueChange,
      [Symbol.dispose]() {},
    }))
    onChange.current = props.onValueChange

    const instance = useDisposable(
      'editor',
      () => {
        queueMicrotask(() => {
          if (!container.current) {
            return
          }
          const model = acquireModel(modelUri, props.value, props.language)
          const editorInstance = editor.create(container.current, {
            model,
            theme: 'vs-dark',
            automaticLayout: true,
            minimap: { enabled: false },
            fontSize: 14,
            scrollBeyondLastLine: false,
            wordWrap: 'on',
          })
          const changeSub = editorInstance.onDidChangeModelContent(() => {
            onChange.current(editorInstance.getValue())
          })
          instance.value = { editorInstance, changeSub }
        })
        const state: {
          value?: {
            editorInstance: editor.IStandaloneCodeEditor
            changeSub: { dispose(): void }
          }
        } = {}
        return {
          get value() {
            return state.value
          },
          set value(next) {
            state.value = next
          },
          [Symbol.dispose]() {
            state.value?.changeSub.dispose()
            state.value?.editorInstance.dispose()
            state.value = undefined
          },
        }
      },
      [],
    )

    useDisposable(
      'sync-model',
      () => {
        const uri = modelUri
        const text = props.value
        const languageId = props.language
        queueMicrotask(() => {
          if (props.path) {
            registerWorkspaceTypeScriptFile(props.path, text)
          }
          const model = acquireModel(uri, text, languageId)
          const editorInstance = instance.value?.editorInstance
          if (editorInstance && editorInstance.getModel() !== model) {
            editorInstance.setModel(model)
          }
          if (model.getLanguageId() !== languageId) {
            editor.setModelLanguage(model, languageId)
          }
          if (model.getValue() !== text) {
            model.setValue(text)
          }
        })
        return { [Symbol.dispose]() {} }
      },
      [props.revision, props.language, props.path],
    )

    useDisposable(
      'berry-markers',
      () => {
        const model = editor.getModel(modelUri)
        if (!model) return { [Symbol.dispose]() {} }
        if (props.language !== 'typescript' || !props.berryDiagnostics) {
          editor.setModelMarkers(model, 'cos-berry', [])
          return { [Symbol.dispose]() {} }
        }
        editor.setModelMarkers(
          model,
          'cos-berry',
          props.berryDiagnostics.map((d) => ({
            startLineNumber: d.line,
            startColumn: d.column,
            endLineNumber: d.line,
            endColumn: d.column + 1,
            message: d.message,
            severity: markerSeverity(d.severity),
            code: d.code,
            source: 'cos-berry',
          })),
        )
        return { [Symbol.dispose]() {} }
      },
      [props.berryDiagnostics, props.language, props.path, props.revision],
    )

  return <div ref={container} className="editor-container" data-spatial-nav-passthrough="" />
  },
})

function markerSeverity(severity: BerryDiagnostic['severity']): MarkerSeverity {
  switch (severity) {
    case 'warning':
      return MarkerSeverity.Warning
    case 'info':
      return MarkerSeverity.Info
    case 'hint':
      return MarkerSeverity.Hint
    default:
      return MarkerSeverity.Error
  }
}

function modelUriForPath(path: string, language: string): Uri {
  if (path) {
    return Uri.parse(modelFilePath(path))
  }
  const extension = language === 'javascript' ? 'js' : 'ts'
  return Uri.parse(`file:///workspace/untitled.${extension}`)
}

function acquireModel(uri: Uri, value: string, language: string): editor.ITextModel {
  const existing = editor.getModel(uri)
  if (existing) {
    return existing
  }
  return editor.createModel(value, language, uri)
}
