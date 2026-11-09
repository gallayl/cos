import { ObservableValue } from '@furystack/utils'
import { describe, expect, it, vi } from 'vitest'

import { createConsoleSession } from './console-session'
import type { ConsoleSocket } from './console-link'

const authorization = `Basic ${btoa('cos:secret')}`

class FakeSocket implements ConsoleSocket {
  static latest: FakeSocket | undefined
  readyState = 0
  onopen: ConsoleSocket['onopen'] = null
  onmessage: ConsoleSocket['onmessage'] = null
  onerror: ConsoleSocket['onerror'] = null
  onclose: ConsoleSocket['onclose'] = null
  sent: string[] = []
  url: string

  constructor(url: string) {
    this.url = url
    FakeSocket.latest = this
  }

  send(data: string) {
    this.sent.push(data)
  }

  close() {
    this.readyState = 3
    this.onclose?.({} as CloseEvent)
  }
}

async function flush(): Promise<void> {
  for (let step = 0; step < 6; step += 1) {
    await Promise.resolve()
  }
}

describe('console session', () => {
  it('sends the draft that was typed, without waiting for a later render', async () => {
    const header = new ObservableValue(authorization)
    const needsAuth = new ObservableValue(false)
    const fetchImpl = vi.fn(async () => new Response('200000', { status: 200 }))
    const session = createConsoleSession({
      authorization: header,
      needsAuth,
      markUnauthorized: () => needsAuth.setValue(true),
      fetchImpl,
      webSocket: FakeSocket,
      pageUrl: () => 'http://localhost/console',
    })

    await flush()
    const socket = FakeSocket.latest
    expect(socket?.url).toBe('ws://cos:secret@localhost/ws')
    socket!.readyState = 1
    socket!.onopen?.({} as Event)

    session.setDraft('ls')
    session.send()

    expect(socket!.sent).toEqual(['ls'])
    expect(session.draft.getValue()).toBe('')
    /* eslint-disable furystack/prefer-using-wrapper -- test disposes multiple observables after async assertions */
    session[Symbol.dispose]()
    header[Symbol.dispose]()
    needsAuth[Symbol.dispose]()
    /* eslint-enable furystack/prefer-using-wrapper */
  })
})
