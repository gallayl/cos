import { createComponent, Shade } from '@furystack/shades'
import { Typography, cssVariableTheme } from '@furystack/shades-common-components'

export interface EditorStatusBarProps {
  status: string
  location: string
  lines: string
}

/** Path, editor status, and line count. */
export const EditorStatusBar = Shade<EditorStatusBarProps>({
  customElementName: 'cos-editor-status-bar',
  css: {
    display: 'block',
    width: '100%',
    '& footer': {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      gap: cssVariableTheme.spacing.md,
      minHeight: '1.75rem',
      padding: `0 ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    '& shade-typography': {
      minWidth: '0',
      overflow: 'hidden',
      textOverflow: 'ellipsis',
      whiteSpace: 'nowrap',
    },
  },
  render: ({ props }) => (
    <footer>
      <Typography variant="caption" color="textSecondary" ellipsis>
        {props.status}
      </Typography>
      <Typography variant="caption" color="textSecondary" ellipsis>
        {props.location}
      </Typography>
      <Typography variant="caption" color="textSecondary">
        {props.lines}
      </Typography>
    </footer>
  ),
})
