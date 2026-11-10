import { createComponent, NestedRouteLink, Shade } from '@furystack/shades'
import {
  Alert,
  Button,
  Input,
  SegmentedControl,
  Typography,
  cssVariableTheme,
} from '@furystack/shades-common-components'

import { annotateBerryErrorText } from '../ide/ast-to-berry'
import { BerryCompileContext } from '../ide/berry-compile-context'
import { DeviceAccess } from '../editor/device-access'
import { DeviceSignIn } from '../editor/device-sign-in'
import { consoleMessageLimit, lineVisible, severityOf, stripAnsi } from './console-lines'
import { ConsoleSession, type ConsolePhase } from './console-session'

const levelOptions = [
  { value: 'error', label: 'Error' },
  { value: 'warn', label: 'Warn' },
  { value: 'info', label: 'Info' },
]

const phaseText: Record<ConsolePhase, string> = {
  'signed-out': 'Sign in to use the console',
  connecting: 'Connecting',
  open: 'Connected',
  unauthorized: 'The device rejected these credentials',
  unreachable: 'Cannot reach the device',
  closed: 'Disconnected',
}

/** Device console. Connection state lives on {@link ConsoleSession}. */
export const ConsolePage = Shade({
  customElementName: 'cos-console-page',
  css: {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
    minHeight: '0',
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    '& header': {
      display: 'flex',
      alignItems: 'center',
      flexWrap: 'wrap',
      gap: cssVariableTheme.spacing.md,
      padding: `${cssVariableTheme.spacing.sm} ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    '& .title': {
      display: 'flex',
      alignItems: 'center',
      gap: cssVariableTheme.spacing.md,
      marginRight: 'auto',
    },
    '& shade-typography': {
      margin: '0',
    },
    '& nested-route-link': {
      color: cssVariableTheme.text.secondary,
      fontSize: cssVariableTheme.typography.fontSize.sm,
    },
    '& .log': {
      flex: '1',
      minHeight: '0',
      overflow: 'auto',
      margin: '0',
      padding: cssVariableTheme.spacing.md,
      fontFamily: 'ui-monospace, monospace',
      fontSize: cssVariableTheme.typography.fontSize.sm,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
      whiteSpace: 'pre-wrap',
      overflowWrap: 'anywhere',
    },
    '& .error': { color: cssVariableTheme.palette.error.main },
    '& .warn': { color: cssVariableTheme.palette.warning.main },
    '& .info': { color: cssVariableTheme.palette.info.main },
    '& .output': { color: cssVariableTheme.text.primary },
    '& .composer': {
      display: 'flex',
      alignItems: 'center',
      gap: cssVariableTheme.spacing.sm,
      padding: cssVariableTheme.spacing.sm,
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.paper,
    },
    '& .composer shade-input': {
      flex: '1',
      margin: '0',
    },
    '& .composer shade-button': {
      margin: '0',
    },
    '& .notice': {
      padding: `0 ${cssVariableTheme.spacing.md}`,
    },
  },
  render: ({ injector, useObservable, useDisposable, useRef }) => {
    const access = injector.get(DeviceAccess)
    const session = injector.get(ConsoleSession)
    const compileContext = injector.get(BerryCompileContext)
    const [authorization] = useObservable('authorization', access.authorization)
    const [needsAuth] = useObservable('needs-auth', access.needsAuth)
    const [phase] = useObservable('phase', session.phase)
    const [transcript] = useObservable('transcript', session.transcript)
    const [level] = useObservable('level', session.level)
    const [draft] = useObservable('draft', session.draft)
    const [compileMetadata] = useObservable('compile-metadata', compileContext.metadata)
    const log = useRef<HTMLDivElement>('log')
    const follow = useDisposable('follow', () => ({
      current: true,
      [Symbol.dispose]() {},
    }))

    const visible = transcript.lines.filter((line) => lineVisible(line.severity, level))
    const pending = stripAnsi(transcript.pending)
    const pendingSeverity = severityOf(pending)
    const showPending = pending !== '' && lineVisible(pendingSeverity, level)
    const mapLine = (text: string) =>
      compileMetadata?.mappings?.length ? annotateBerryErrorText(text, compileMetadata) : text
    if (log.current && follow.current) {
      log.current.scrollTop = log.current.scrollHeight
    }

    const signedOut = !authorization || needsAuth
    const connected = phase === 'open'

    return (
      <>
        <header>
          <div className="title">
            <NestedRouteLink path="/">Home</NestedRouteLink>
            <Typography variant="subtitle2">Console</Typography>
            <Typography variant="caption" color="textSecondary">
              {phaseText[phase]}
            </Typography>
            {compileMetadata?.mappings?.length ? (
              <Typography variant="caption" color="textSecondary">
                TS source map active ({compileMetadata.mappings.length})
              </Typography>
            ) : null}
          </div>
          <Typography variant="caption" color="textSecondary">
            Log level
          </Typography>
          <SegmentedControl
            size="small"
            value={level}
            options={levelOptions}
            onValueChange={(value) => session.setLevel(value)}
          />
        </header>
        {signedOut ? <DeviceSignIn /> : null}
        {phase === 'unreachable' || phase === 'closed' ? (
          <div className="notice">
            <Alert severity="error" variant="outlined" title={phaseText[phase]}>
              <Button type="button" size="small" variant="outlined" onclick={() => session.retry()}>
                Retry
              </Button>
            </Alert>
          </div>
        ) : null}
        <div
          className="log"
          ref={log}
          onscroll={(event) => {
            const element = event.currentTarget as HTMLDivElement
            follow.current = element.scrollHeight - element.scrollTop - element.clientHeight < 48
          }}
        >
          {visible.map((line) => (
            <div className={line.severity ?? 'output'}>
              {mapLine(line.text.length > 0 ? line.text : ' ')}
            </div>
          ))}
          {showPending ? <div className={pendingSeverity ?? 'output'}>{mapLine(pending)}</div> : null}
        </div>
        <form
          className="composer"
          oninput={(event) => {
            const { target } = event
            if (target instanceof HTMLInputElement) {
              session.setDraft(target.value)
            }
          }}
          onsubmit={(event) => {
            event.preventDefault()
            session.send()
          }}
          onkeydown={(event) => {
            if (event.key === 'Enter') {
              event.preventDefault()
              session.send()
            }
          }}
        >
          <Input
            type="text"
            variant="outlined"
            size="small"
            placeholder="Command"
            maxLength={consoleMessageLimit}
            disabled={!connected}
            value={draft}
            oninput={(event) => session.setDraft((event.target as HTMLInputElement).value)}
          />
          <Button type="submit" variant="contained" size="small" disabled={!connected || draft.trim() === ''}>
            Send
          </Button>
        </form>
      </>
    )
  },
})
