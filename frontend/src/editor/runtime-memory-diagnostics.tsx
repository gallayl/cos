import { createComponent, Shade } from '@furystack/shades'
import { Button, Paper, cssVariableTheme } from '@furystack/shades-common-components'

import { DeviceAccess } from './device-access'

type MemoryStatus = 'idle' | 'loading' | 'ok' | 'unauthorized' | 'unreachable' | 'error'

interface MemoryState {
  status: MemoryStatus
  freeHeapBytes?: number
  raw?: string
  message?: string
  updatedAt?: number
}

const initialState: MemoryState = { status: 'idle' }

function formatBytes(value: number): string {
  if (!Number.isFinite(value) || value < 0) return `${value} B`
  if (value >= 1024 * 1024) return `${(value / (1024 * 1024)).toFixed(2)} MiB (${value} B)`
  if (value >= 1024) return `${(value / 1024).toFixed(1)} KiB (${value} B)`
  return `${value} B`
}

/**
 * Editor memory diagnostics from runtime metrics (R14).
 * Confirmed frontend-accessible endpoint: GET /api/heap (HTTP Basic auth), text/plain free heap bytes.
 */
export const RuntimeMemoryDiagnostics = Shade({
  customElementName: 'cos-runtime-memory-diagnostics',
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
    '& .hint, & .meta': {
      fontSize: cssVariableTheme.typography.fontSize.sm,
      color: cssVariableTheme.text.secondary,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
    },
    '& .value': {
      fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
      fontSize: cssVariableTheme.typography.fontSize.md,
      color: cssVariableTheme.text.primary,
    },
    '& .actions': {
      display: 'flex',
      gap: cssVariableTheme.spacing.sm,
      alignItems: 'center',
      flexWrap: 'wrap',
    },
    '& label': {
      display: 'inline-flex',
      alignItems: 'center',
      gap: cssVariableTheme.spacing.xs,
      fontSize: cssVariableTheme.typography.fontSize.sm,
      color: cssVariableTheme.text.secondary,
      userSelect: 'none',
    },
  },
  render: ({ useState, useDisposable, injector }) => {
    const [state, setState] = useState<MemoryState>('runtime-memory-state', initialState)
    const [auto, setAuto] = useState('runtime-memory-auto', false)
    const access = injector.get(DeviceAccess)

    const fetchHeap = async () => {
      const authorization = access.authorization.getValue()
      if (!authorization) {
        setState({ status: 'unauthorized', message: 'Sign in to device to fetch runtime memory metrics.' })
        return
      }
      setState({ ...state, status: 'loading', message: undefined })
      try {
        const response = await fetch('/api/heap', { headers: { Authorization: authorization } })
        if (response.status === 401) {
          access.markUnauthorized()
          setState({ status: 'unauthorized', message: 'Device authentication required (401).' })
          return
        }
        if (!response.ok) {
          setState({ status: 'error', message: `Request failed: HTTP ${response.status}` })
          return
        }
        const raw = (await response.text()).trim()
        const freeHeapBytes = Number.parseInt(raw, 10)
        if (!Number.isFinite(freeHeapBytes)) {
          setState({ status: 'error', raw, message: 'Unexpected /api/heap response (expected integer bytes).' })
          return
        }
        setState({ status: 'ok', freeHeapBytes, raw, updatedAt: Date.now() })
      } catch (error) {
        setState({
          status: 'unreachable',
          message: error instanceof Error ? error.message : 'Device unreachable.',
        })
      }
    }

    useDisposable(
      'runtime-memory-poll',
      () => {
        if (!auto) return { [Symbol.dispose]() {} }
        void fetchHeap()
        const id = window.setInterval(() => void fetchHeap(), 5000)
        return { [Symbol.dispose]: () => window.clearInterval(id) }
      },
      [auto, access.authorization.getValue()],
    )

    const statusText =
      state.status === 'ok' && state.freeHeapBytes !== undefined
        ? `Free heap: ${formatBytes(state.freeHeapBytes)}`
        : state.status === 'loading'
          ? 'Fetching runtime memory metrics...'
          : state.status === 'unauthorized'
            ? (state.message ?? 'Unauthorized')
            : state.status === 'unreachable'
              ? `Unreachable: ${state.message ?? 'device not reachable'}`
              : state.status === 'error'
                ? (state.message ?? 'Failed to read runtime memory metrics')
                : 'No runtime memory sample yet.'

    return (
      <Paper elevation={0}>
        <div className="content">
          <div className="title">Runtime memory diagnostics</div>
          <div className="hint">
            Confirmed endpoint: <code>GET /api/heap</code> (HTTP Basic auth, <code>text/plain</code> free heap bytes).
            Values are device runtime metrics; use during/after app runs to observe pressure.
          </div>
          <div className="value">{statusText}</div>
          {state.updatedAt ? (
            <div className="meta">Last updated: {new Date(state.updatedAt).toLocaleTimeString()}</div>
          ) : null}
          {state.status === 'ok' && state.raw ? <div className="meta">Raw response: {state.raw}</div> : null}
          <div className="actions">
            <Button variant="outlined" onclick={() => void fetchHeap()} disabled={state.status === 'loading'}>
              Refresh
            </Button>
            <label>
              <input
                type="checkbox"
                checked={auto}
                onchange={(ev) => setAuto((ev.target as HTMLInputElement).checked)}
              />
              Auto-refresh (5s)
            </label>
          </div>
        </div>
      </Paper>
    )
  },
})
