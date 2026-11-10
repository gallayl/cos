import type { BerryCompileMetadata, BerrySourceLocation, BerrySourceMapEntry } from './diagnostics'

function isAfter(a: BerrySourceLocation, b: BerrySourceLocation): boolean {
  return a.line > b.line || (a.line === b.line && a.column > b.column)
}

function isBeforeOrEqual(a: BerrySourceLocation, b: BerrySourceLocation): boolean {
  return a.line < b.line || (a.line === b.line && a.column <= b.column)
}

/** Find the best mapping for a generated Berry location (1-based): latest mapping at/before the position. */
export function findSourceLocation(
  metadata: BerryCompileMetadata | undefined,
  generated: BerrySourceLocation,
): BerrySourceMapEntry | undefined {
  const mappings = metadata?.mappings
  if (!mappings || mappings.length === 0) return undefined
  let best: BerrySourceMapEntry | undefined
  for (const entry of mappings) {
    if (!isBeforeOrEqual(entry.generated, generated)) continue
    if (!best || isAfter(entry.generated, best.generated)) best = entry
  }
  return best
}

/** Map a Berry runtime/compiler line (1-based) to a TypeScript location when metadata is available. */
export function mapBerryLineToSource(
  metadata: BerryCompileMetadata | undefined,
  berryLine: number,
): BerrySourceLocation | undefined {
  return findSourceLocation(metadata, { line: berryLine, column: 1 })?.source
}

/** Format a mapped location for diagnostics/UI labels. */
export function formatMappedSource(
  metadata: BerryCompileMetadata | undefined,
  berryLine: number,
): string | undefined {
  const mapped = mapBerryLineToSource(metadata, berryLine)
  if (!mapped) return undefined
  const file = metadata?.sourceFileName ? `${metadata.sourceFileName}:` : ''
  return `${file}${mapped.line}:${mapped.column}`
}

function extractBerryLine(line: string): number | undefined {
  let berryLine: number | undefined
  const patterns = [
    // /path/file.be:12:3:, file.be:12:, or :12:
    /(?:^|[\s(,:])(?:[A-Za-z]:)?[^\s():,]*?\.be:(\d+)(?::\d+)?(?=[\s):,]|$)/g,
    /(?:^|[\s(,]):(\d+):(?:\d+:?)?(?=[\s),]|$)/g,
    // "line 12", "line 12, column 3", "at line 12"
    /\b(?:at\s+)?line\s+(\d+)(?:\s*[,:]\s*(?:col(?:umn)?\.?\s*)?\d+)?\b/gi,
  ]
  for (const pattern of patterns) {
    let match: RegExpExecArray | null
    while ((match = pattern.exec(line)) !== null) {
      const value = Number(match[1])
      if (Number.isInteger(value) && value > 0) berryLine = value
    }
  }
  return berryLine
}

/**
 * Annotate Berry/runtime/compiler error text with mapped TypeScript locations when metadata is available.
 * This is a best-effort foundation for runtime-error UX; it does not modify lines without a detectable Berry line.
 */
export function annotateBerryErrorText(
  text: string,
  metadata: BerryCompileMetadata | undefined,
): string {
  if (!metadata?.mappings?.length) return text
  return text
    .split('\n')
    .map((line) => {
      const berryLine = extractBerryLine(line)
      if (!berryLine) return line
      const mapped = formatMappedSource(metadata, berryLine)
      if (!mapped) return line
      const suffix = `(ts ${mapped}, berry line ${berryLine})`
      if (line.includes(suffix)) return line
      return `${line} ${suffix}`
    })
    .join('\n')
}
