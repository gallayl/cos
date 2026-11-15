declare module '@cos/system' {
  export interface MemInfoOk {
    ok: true;
    free_heap: number;
    min_free_heap: number;
    total_heap: number;
    max_alloc_block: number;
    internal_free_heap: number;
    internal_min_free_heap: number;
    internal_total_heap: number;
    internal_max_alloc_block: number;
  }

  export interface MemInfoFailed {
    ok: false;
    error: string;
  }

  export function meminfo(): MemInfoOk | MemInfoFailed;
  export function uptime(): string;

  const system: {
    meminfo: typeof meminfo;
    uptime: typeof uptime;
  };
  export default system;
}

declare module '@berry/system' {
  export * from '@cos/system';
  import _default from '@cos/system';
  export default _default;
}
