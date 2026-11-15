import { createComponent, Shade } from '@furystack/shades'
import { cssVariableTheme } from '@furystack/shades-common-components'

import { DeviceAccess } from '../editor/device-access'

type MemoryStatus = 'idle' | 'loading' | 'ok' | 'unauthorized' | 'unreachable' | 'error'

interface MemoryState {
  status: MemoryStatus
  freeHeapBytes?: number
  message?: string
  updatedAt?: number
}

function formatBytes(value: number): string {
  if (!Number.isFinite(value) || value < 0) return `${value} B`
  if (value >= 1024 * 1024) return `${(value / (1024 * 1024)).toFixed(2)} MiB`
  if (value >= 1024) return `${(value / 1024).toFixed(1)} KiB`
  return `${value} B`
}

/** Compact, always-updating device memory indicator for the global footer. */
export const MemoryFootprint = Shade({
  customElementName: 'cos-memory-footprint',
  css: {
    display: 'inline-flex',
    alignItems: 'center',
    gap: cssVariableTheme.spacing.sm,
    color: cssVariableTheme.text.secondary,
    fontSize: cssVariableTheme.typography.fontSize.xs,
    whiteSpace: 'nowrap',
    '& .label': {
      color: cssVariableTheme.text.secondary,
    },
    '& .value': {
      fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
      color: cssVariableTheme.text.primary,
    },
  },
  render: ({ injector, useDisposable, useState }) => {
    const access = injector.get(DeviceAccess)
    const [state, setState] = useState<MemoryState>('memory', { status: 'idle' })

    const fetchHeap = async () => {
      const authorization = access.authorization.getValue()
      if (!authorization) {
        setState({ status: 'unauthorized', message: 'Sign in for memory' })
        return
      }
      try {
        const response = await fetch('/api/heap', { headers: { Authorization: authorization } })
        if (response.status === 401) {
          access.markUnauthorized()
          setState({ status: 'unauthorized', message: 'Auth required' })
          return
        }
        if (!response.ok) {
          setState({ status: 'error', message: `HTTP ${response.status}` })
          return
        }
        const raw = (await response.text()).trim()
        const freeHeapBytes = Number.parseInt(raw, 10)
        if (!Number.isFinite(freeHeapBytes)) {
          setState({ status: 'error', message: 'Bad metric' })
          return
        }
        setState({ status: 'ok', freeHeapBytes, updatedAt: Date.now() })
      } catch {
        setState({ status: 'unreachable', message: 'Unreachable' })
      }
    }

    useDisposable(
      'memory-poll',
      () => {
        void fetchHeap()
        const id = window.setInterval(() => void fetchHeap(), 5000)
        return { [Symbol.dispose]: () => window.clearInterval(id) }
      },
      [access.authorization.getValue()],
    )

    const text =
      state.status === 'ok' && state.freeHeapBytes !== undefined
        ? `${formatBytes(state.freeHeapBytes)} free`
        : state.status === 'loading'
          ? 'Loading…'
          : (state.message ?? 'Memory n/a')

    return (
      <span title={state.updatedAt ? `Updated ${new Date(state.updatedAt).toLocaleTimeString()}` : 'Runtime memory'}>
        <span className="label">Memory: </span>
        <span className="value">{text}</span>
      </span>
    )
  },
})
