import { createComponent, Shade } from '@furystack/shades'
import { Button, Paper, cssVariableTheme } from '@furystack/shades-common-components'

import { annotateBerryErrorText, type BerryCompileMetadata } from '../ide/ast-to-berry'

export interface BerryErrorAnnotatorProps {
  metadata?: BerryCompileMetadata
}

/** Paste Berry/runtime/compiler error text and annotate it with mapped TypeScript locations. */
export const BerryErrorAnnotator = Shade<BerryErrorAnnotatorProps>({
  customElementName: 'cos-berry-error-annotator',
  css: {
    display: 'block',
    margin: `0 ${cssVariableTheme.spacing.md} ${cssVariableTheme.spacing.md}`,
    '& .content': {
      display: 'flex',
      flexDirection: 'column',
      gap: cssVariableTheme.spacing.sm,
    },
    '& .title': {
      fontSize: cssVariableTheme.typography.fontSize.sm,
      fontWeight: '600',
      color: cssVariableTheme.text.primary,
    },
    '& .hint': {
      fontSize: cssVariableTheme.typography.fontSize.sm,
      color: cssVariableTheme.text.secondary,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
    },
    '& textarea, & pre': {
      width: '100%',
      boxSizing: 'border-box',
      margin: '0',
      padding: cssVariableTheme.spacing.sm,
      borderRadius: cssVariableTheme.shape.borderRadius.md,
      border: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.default,
      color: cssVariableTheme.text.primary,
      fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
      fontSize: cssVariableTheme.typography.fontSize.sm,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
    },
    '& textarea': {
      minHeight: '5rem',
      resize: 'vertical',
    },
    '& pre': {
      maxHeight: '10rem',
      overflow: 'auto',
      whiteSpace: 'pre-wrap',
      wordBreak: 'break-word',
    },
    '& .actions': {
      display: 'flex',
      gap: cssVariableTheme.spacing.sm,
      alignItems: 'center',
      flexWrap: 'wrap',
    },
  },
  render: ({ props, useState }) => {
    const [input, setInput] = useState('berry-error-input', '')
    const [annotated, setAnnotated] = useState('berry-error-annotated', '')
    const mappings = props.metadata?.mappings?.length ?? 0
    const canMap = mappings > 0
    const profile = props.metadata?.profile ?? 'dev'

    const run = () => {
      setAnnotated(annotateBerryErrorText(input, props.metadata))
    }

    return (
      <Paper elevation={0}>
        <div className="content">
          <div className="title">Map Berry error/output to TypeScript</div>
          <div className="hint">
            {canMap
              ? `Uses last compile metadata (${profile}, ${mappings} mappings). Paste Berry/runtime/compiler text containing line numbers (for example "file.be:12:" or "line 12") to annotate with TS locations.`
              : `No source mappings available from the last compile (${profile}). Compile in dev profile to generate mappings.`}
          </div>
          <textarea
            value={input}
            placeholder={'Paste Berry error/output here, e.g.\nsyntax_error: /virtual/app.be:12: expected end'}
            oninput={(ev) => setInput((ev.target as HTMLTextAreaElement).value)}
            disabled={!canMap}
          />
          <div className="actions">
            <Button variant="outlined" onclick={run} disabled={!canMap || input.trim() === ''}>
              Annotate
            </Button>
            <Button
              variant="text"
              onclick={() => {
                setInput('')
                setAnnotated('')
              }}
              disabled={input === '' && annotated === ''}
            >
              Clear
            </Button>
          </div>
          {annotated ? <pre>{annotated}</pre> : null}
        </div>
      </Paper>
    )
  },
})
