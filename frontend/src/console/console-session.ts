import { defineService } from '@furystack/inject'
import { ObservableValue } from '@furystack/utils'

import { DeviceAccess } from '../editor/device-access'
import { openConsoleLink, type ConsoleLink, type ConsoleLinkPhase, type ConsoleSocketFactory } from './console-link'
import {
  createConsoleTranscript,
  isConsoleSeverity,
  normalizeConsoleMessage,
  pushConsoleChunk,
  type ConsoleSeverity,
  type ConsoleTranscript,
} from './console-lines'

/** Connection state shown by the console page. */
export type ConsolePhase = ConsoleLinkPhase | 'signed-out'

/** Live console session. The page observes these values and calls {@link ConsoleSessionService.send}. */
export interface ConsoleSessionService extends Disposable {
  readonly phase: ObservableValue<ConsolePhase>
  readonly transcript: ObservableValue<ConsoleTranscript>
  readonly level: ObservableValue<ConsoleSeverity>
  readonly draft: ObservableValue<string>
  /** Replace the log threshold. Does not change the device setting. */
  setLevel(level: string): void
  /** Remember the command field as the user types. */
  setDraft(value: string): void
  /** Send the current draft as one shell line. */
  send(): void
  /** Open the websocket again after a disconnect. */
  retry(): void
}

export interface ConsoleSessionOptions {
  authorization: ObservableValue<string>
  needsAuth: ObservableValue<boolean>
  markUnauthorized: () => void
  fetchImpl?: typeof fetch
  webSocket?: ConsoleSocketFactory
  pageUrl?: () => string
}

/**
 * Device console session.
 * Uses the same Basic header as the filesystem API. The websocket handshake
 * carries that header because the browser cannot set one on `WebSocket` directly.
 */
export function createConsoleSession(options: ConsoleSessionOptions): ConsoleSessionService {
  const phase = new ObservableValue<ConsolePhase>('signed-out')
  const transcript = new ObservableValue(createConsoleTranscript())
  const level = new ObservableValue<ConsoleSeverity>('warn')
  const draft = new ObservableValue('')
  let generation = 0
  let link: ConsoleLink | null = null

  const connect = () => {
    const current = ++generation
    link?.close()
    link = null
    const authorization = options.authorization.getValue()
    if (!authorization || options.needsAuth.getValue()) {
      phase.setValue(authorization ? 'unauthorized' : 'signed-out')
      return
    }

    void openConsoleLink({
      authorization,
      pageUrl: options.pageUrl?.() ?? window.location.href,
      fetchImpl: options.fetchImpl,
      webSocket: options.webSocket,
      onChunk: (chunk) => {
        if (current !== generation) {
          return
        }
        transcript.setValue(pushConsoleChunk(transcript.getValue(), chunk))
      },
      onPhase: (next) => {
        if (current !== generation) {
          return
        }
        if (next === 'unauthorized') {
          options.markUnauthorized()
        }
        phase.setValue(next)
      },
    }).then((opened) => {
      if (current !== generation) {
        opened.close()
        return
      }
      link = opened
    })
  }

  const authorizationChanges = options.authorization.subscribe(connect)
  const authStateChanges = options.needsAuth.subscribe(connect)
  connect()

  return {
    phase,
    transcript,
    level,
    draft,
    setLevel(value) {
      if (isConsoleSeverity(value)) {
        level.setValue(value)
      }
    },
    setDraft(value) {
      draft.setValue(value)
    },
    send() {
      const message = normalizeConsoleMessage(draft.getValue())
      if (!message || !link?.send(message)) {
        return
      }
      draft.setValue('')
    },
    retry: connect,
    [Symbol.dispose]() {
      generation += 1
      link?.close()
      /* eslint-disable furystack/prefer-using-wrapper -- These subscriptions and ObservableValues are created here but owned by the returned session; disposal is deferred to the session's lifetime, so a same-scope using() wrapper cannot express this ownership transfer. */
      authorizationChanges[Symbol.dispose]()
      authStateChanges[Symbol.dispose]()
      phase[Symbol.dispose]()
      transcript[Symbol.dispose]()
      level[Symbol.dispose]()
      draft[Symbol.dispose]()
      /* eslint-enable furystack/prefer-using-wrapper */
    },
  }
}

/** Console session for the signed-in device. */
export const ConsoleSession = defineService({
  name: 'cos/ConsoleSession',
  lifetime: 'singleton',
  factory: ({ injector, onDispose }) => {
    const access = injector.get(DeviceAccess)
    const session = createConsoleSession({
      authorization: access.authorization,
      needsAuth: access.needsAuth,
      markUnauthorized: () => access.markUnauthorized(),
    })
    onDispose(() => session[Symbol.dispose]())
    return session
  },
})
