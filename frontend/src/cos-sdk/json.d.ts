/**
 * Berry `json` module.
 * `load` parses a JSON string. `dump` serializes a value; pass `"format"` as the
 * second argument for indented output.
 */
declare global {
  namespace json {
    function load(text: string): any;
    function dump(value: any, format?: "format"): string;
  }
}

export {};
