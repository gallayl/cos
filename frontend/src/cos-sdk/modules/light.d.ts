declare module '@cos/light' {
  export interface ReadOk {
    ok: true;
    /** Raw sensor value (0-4095). */
    value: number;
  }

  export interface ReadFailed {
    ok: false;
    error: string;
  }

  export function read(): ReadOk | ReadFailed;

  const light: {
    read: typeof read;
  };
  export default light;
}

declare module '@berry/light' {
  export * from '@cos/light';
  import _default from '@cos/light';
  export default _default;
}
