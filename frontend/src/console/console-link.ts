/** Websocket lifecycle shown by the console page. */
export type ConsoleLinkPhase = 'connecting' | 'open' | 'unauthorized' | 'unreachable' | 'closed'

/** Minimal socket used by {@link openConsoleLink}. */
export interface ConsoleSocket {
  readyState: number
  onopen: ((event: Event) => void) | null
  onmessage: ((event: MessageEvent) => void) | null
  onerror: ((event: Event) => void) | null
  onclose: ((event: CloseEvent) => void) | null
  send(data: string): void
  close(): void
}

/** Constructor compatible with the browser `WebSocket`. */
export type ConsoleSocketFactory = new (url: string) => ConsoleSocket

/** An open console socket. `send` forwards one text frame. */
export interface ConsoleLink {
  send(text: string): boolean
  close(): void
}

export interface OpenConsoleLinkOptions {
  /** `Authorization` header value, including the `Basic ` prefix. */
  authorization: string
  /** Page URL used to build the `/ws` address. */
  pageUrl: string
  fetchImpl?: typeof fetch
  webSocket?: ConsoleSocketFactory
  onChunk: (chunk: string) => void
  onPhase: (phase: ConsoleLinkPhase) => void
}

const webSocketOpen = 1

const idleLink: ConsoleLink = {
  send: () => false,
  close() {},
}

/** Text from a websocket message. Binary frames are decoded; anything else is ignored. */
export async function consoleSocketText(data: unknown): Promise<string | null> {
  if (typeof data === 'string') {
    return data
  }
  if (data instanceof Blob) {
    return data.text()
  }
  if (data instanceof ArrayBuffer) {
    return new TextDecoder().decode(data)
  }
  return null
}

/** Decode the username and password stored in an HTTP Basic header. */
export function credentialsFromAuthorization(authorization: string): { username: string; password: string } | null {
  if (!authorization.startsWith('Basic ')) {
    return null
  }
  try {
    const decoded = atob(authorization.slice('Basic '.length))
    const separator = decoded.indexOf(':')
    if (separator <= 0) {
      return null
    }
    return { username: decoded.slice(0, separator), password: decoded.slice(separator + 1) }
  } catch {
    return null
  }
}

/**
 * WebSocket URL for the device console.
 * Credentials are placed in the URL so the browser sends HTTP Basic auth on the handshake.
 */
export function consoleWebSocketUrl(pageUrl: string, authorization: string): string | null {
  const credentials = credentialsFromAuthorization(authorization)
  if (!credentials) {
    return null
  }
  const url = new URL('/ws', pageUrl)
  url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:'
  url.username = credentials.username
  url.password = credentials.password
  return url.href
}

/** Check the device credentials with a small authenticated request. */
export async function probeDeviceAuth(
  authorization: string,
  fetchImpl: typeof fetch = fetch,
): Promise<'ok' | 'unauthorized' | 'unreachable'> {
  try {
    const response = await fetchImpl('/api/heap', { headers: { Authorization: authorization } })
    if (response.status === 401) {
      return 'unauthorized'
    }
    if (!response.ok) {
      return 'unreachable'
    }
    return 'ok'
  } catch {
    return 'unreachable'
  }
}

/**
 * Open the device console websocket.
 * The handshake uses the same HTTP Basic credentials as the filesystem API.
 */
export async function openConsoleLink(options: OpenConsoleLinkOptions): Promise<ConsoleLink> {
  const notify = (phase: ConsoleLinkPhase) => {
    if (!closed) {
      options.onPhase(phase)
    }
  }
  let closed = false
  await Promise.resolve()
  if (!options.authorization) {
    notify('unauthorized')
    return idleLink
  }

  notify('connecting')
  const probe = await probeDeviceAuth(options.authorization, options.fetchImpl)
  if (closed) {
    return idleLink
  }
  if (probe === 'unauthorized') {
    notify('unauthorized')
    return idleLink
  }
  if (probe === 'unreachable') {
    notify('unreachable')
    return idleLink
  }

  const url = consoleWebSocketUrl(options.pageUrl, options.authorization)
  if (!url) {
    notify('unauthorized')
    return idleLink
  }

  const SocketImpl = options.webSocket ?? WebSocket
  const socket = new SocketImpl(url)
  let opened = false
  socket.onopen = () => {
    opened = true
    notify('open')
  }
  socket.onmessage = (event) => {
    void consoleSocketText(event.data).then((text) => {
      if (text != null && !closed) {
        options.onChunk(text)
      }
    })
  }
  socket.onerror = () => {
    if (!opened) {
      notify('unreachable')
    }
  }
  socket.onclose = () => {
    notify(opened ? 'closed' : 'unreachable')
  }

  return {
    send(text: string) {
      if (closed || socket.readyState !== webSocketOpen) {
        return false
      }
      socket.send(text)
      return true
    },
    close() {
      closed = true
      socket.close()
    },
  }
}
