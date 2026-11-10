/** Indent-aware Berry source buffer. Berry statements are separated by newlines. */
export class BerryWriter {
  private lines: string[] = []
  private level = 0

  writeLine(text = ''): void {
    const pad = '    '.repeat(this.level)
    for (const line of text.split('\n')) {
      this.lines.push(line.length === 0 ? '' : `${pad}${line}`)
    }
  }

  indent(): void {
    this.level += 1
  }

  dedent(): void {
    if (this.level > 0) this.level -= 1
  }

  /**
   * @brief Run `build` at column 0 and return the text it wrote.
   * The caller inserts that text into an expression, then the surrounding
   * statement indent is applied once.
   */
  capture(build: () => void): string {
    const { lines } = this
    const { level } = this
    this.lines = []
    this.level = 0
    build()
    const captured = this.lines.join('\n')
    this.lines = lines
    this.level = level
    return captured
  }

  toString(): string {
    if (this.lines.length === 0) return ''
    return `${this.lines.join('\n')}\n`
  }
}
