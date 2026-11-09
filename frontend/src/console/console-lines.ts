/** Device log thresholds, from most severe to most verbose. */
export const consoleSeverities = ['error', 'warn', 'info'] as const

/** Same levels the device `logging` command accepts. */
export type ConsoleSeverity = (typeof consoleSeverities)[number]

/** True when `value` is one of the device log thresholds. */
export function isConsoleSeverity(value: string): value is ConsoleSeverity {
  return (consoleSeverities as readonly string[]).includes(value)
}

/** One completed console row. `severity` is null for command output and prompts. */
export interface ConsoleLine {
  id: number
  text: string
  severity: ConsoleSeverity | null
}

/** Completed rows plus a fragment that has not ended in a newline yet. */
export interface ConsoleTranscript {
  lines: ConsoleLine[]
  pending: string
  nextId: number
}

/** Longest command the device shell accepts, excluding the newline. */
export const consoleMessageLimit = 126

/** Completed rows kept in the browser. */
export const consoleLineLimit = 200

const severityRank: Record<ConsoleSeverity, number> = {
  error: 0,
  warn: 1,
  info: 2,
}

// eslint-disable-next-line no-control-regex -- ANSI escape sequences intentionally contain ESC (0x1b)
const ansiPattern = /\u001b\[[0-9;]*m/g
const logLinePattern = /^([EWI]) \(\d+\) /

/** Empty transcript. */
export function createConsoleTranscript(): ConsoleTranscript {
  return { lines: [], pending: '', nextId: 1 }
}

/** Remove ESP-IDF color sequences. */
export function stripAnsi(text: string): string {
  return text.replace(ansiPattern, '')
}

/**
 * Severity of one ESP-IDF log line.
 * Command output and prompts return null and stay visible at every threshold.
 */
export function severityOf(text: string): ConsoleSeverity | null {
  const match = stripAnsi(text).match(logLinePattern)
  if (match?.[1] === 'E') {
    return 'error'
  }
  if (match?.[1] === 'W') {
    return 'warn'
  }
  if (match?.[1] === 'I') {
    return 'info'
  }
  return null
}

/** True when a row should be shown for the selected threshold. */
export function lineVisible(severity: ConsoleSeverity | null, threshold: ConsoleSeverity): boolean {
  if (severity === null) {
    return true
  }
  return severityRank[severity] <= severityRank[threshold]
}

/**
 * One shell line to send, or null when there is nothing to send.
 * Newlines are removed so Enter and Send each submit a single message.
 */
export function normalizeConsoleMessage(text: string): string | null {
  const message = text.replaceAll(/[\r\n]/g, '').trim()
  if (message === '') {
    return null
  }
  return message.slice(0, consoleMessageLimit)
}

/** Append a socket chunk, splitting on newlines. */
export function pushConsoleChunk(transcript: ConsoleTranscript, chunk: string): ConsoleTranscript {
  const combined = transcript.pending + chunk
  const parts = combined.split('\n')
  const pending = capPending(parts.pop() ?? '')
  let { lines } = transcript
  let { nextId } = transcript
  for (const part of parts) {
    const text = stripAnsi(part.replace(/\r$/, ''))
    lines = appendConsoleLine(lines, { id: nextId, text, severity: severityOf(text) })
    nextId += 1
  }
  return { lines, pending, nextId }
}

function appendConsoleLine(lines: ConsoleLine[], line: ConsoleLine): ConsoleLine[] {
  const next = [...lines, line]
  if (next.length <= consoleLineLimit) {
    return next
  }
  return next.slice(next.length - consoleLineLimit)
}

function capPending(pending: string): string {
  if (pending.length <= 1024) {
    return pending
  }
  return pending.slice(pending.length - 1024)
}
