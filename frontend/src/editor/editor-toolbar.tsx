import { createComponent, NestedRouteLink, Shade } from '@furystack/shades'
import { Button, ButtonGroup, Typography, cssVariableTheme } from '@furystack/shades-common-components'

export interface EditorToolbarProps {
  canSave: boolean
  canPreview: boolean
  showCompile: boolean
  canCompile: boolean
  onSave: () => void
  onCompile: () => void
  onPreview: () => void
}

/** Primary editor actions. */
export const EditorToolbar = Shade<EditorToolbarProps>({
  customElementName: 'cos-editor-toolbar',
  css: {
    display: 'block',
    width: '100%',
    '& header': {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      gap: cssVariableTheme.spacing.md,
      minHeight: '3rem',
      padding: `${cssVariableTheme.spacing.sm} ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    '& .title': {
      display: 'flex',
      alignItems: 'center',
      gap: cssVariableTheme.spacing.md,
      minWidth: '0',
    },
    '& shade-typography': {
      margin: '0',
    },
    '& nested-route-link': {
      color: cssVariableTheme.text.secondary,
      fontSize: cssVariableTheme.typography.fontSize.sm,
      whiteSpace: 'nowrap',
    },
    '& shade-button-group': {
      flexWrap: 'wrap',
      justifyContent: 'flex-end',
    },
  },
  render: ({ props }) => {
    const disabledStyle = { opacity: cssVariableTheme.action.disabledOpacity, pointerEvents: 'none' as const }
    return (
      <header>
        <div className="title">
          <NestedRouteLink path="/">Home</NestedRouteLink>
          <Typography variant="subtitle2">COS TypeScript</Typography>
        </div>
        <ButtonGroup variant="outlined">
          <Button
            type="button"
            size="small"
            onclick={props.onSave}
            aria-disabled={props.canSave ? 'false' : 'true'}
            style={props.canSave ? undefined : disabledStyle}
          >
            Save
          </Button>
          <Button
            type="button"
            size="small"
            onclick={props.onPreview}
            aria-disabled={props.canPreview ? 'false' : 'true'}
            style={props.canPreview ? undefined : disabledStyle}
          >
            Preview
          </Button>
          {props.showCompile ? (
            <Button
              type="button"
              size="small"
              onclick={props.onCompile}
              aria-disabled={props.canCompile ? 'false' : 'true'}
              style={props.canCompile ? undefined : disabledStyle}
            >
              Compile
            </Button>
          ) : null}
        </ButtonGroup>
      </header>
    )
  },
})
