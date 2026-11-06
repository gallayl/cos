/**
 * Tracks lexical names so a binding that would change meaning under Berry's
 * single function scope is rejected instead of silently overwritten.
 */
export class ScopeTracker {
  private readonly stack: Array<Set<string>> = [new Set()];

  push(): void {
    this.stack.push(new Set());
  }

  pop(): void {
    if (this.stack.length > 1) this.stack.pop();
  }

  /**
   * @brief Record `name` in the current scope.
   * @return False when the name is already visible.
   */
  declare(name: string): boolean {
    for (const scope of this.stack) {
      if (scope.has(name)) return false;
    }
    this.stack[this.stack.length - 1]?.add(name);
    return true;
  }

  isDeclared(name: string): boolean {
    return this.stack.some((scope) => scope.has(name));
  }
}
