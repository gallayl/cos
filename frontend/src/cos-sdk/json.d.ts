/**
 * Berry `json` module.
 * `load` parses a JSON string. `dump` serializes a value; pass `"format"` as the
 * second argument for indented output.
 *
 * Runtime note (R1/R6): `json` is loaded on demand. It is available as a module
 * (`import json from '@cos/json'` / Berry `import json`) and is installed as a
 * global by runtime JSON paths (e.g. `fs.read_json`/`write_json`, `net.fetch`
 * JSON parsing). Prefer an explicit module import in new TypeScript code.
 */
declare global {
  namespace json {
    function load(text: string): any;
    function dump(value: any, format?: "format"): string;
  }
}

export {};
