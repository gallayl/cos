import console from "@cos/console";

export default class PipeStdin implements CosApplication {
  run(_args: string[]): number {
    const input = console.stdin();
    if (input === null) {
      log("stdin: null");
      return 0;
    }

    log("stdin: " + input);
    return 0;
  }
}
