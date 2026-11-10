import console from "@cos/console";

export default class Prompt implements CosApplication {
  greet(name: string | null): void {
    if (name === null) {
      log("Input cancelled");
    } else {
      log("Hello, " + name);
    }
  }

  run(_args: string[]): number {
    const result = console.read_line("Name: ", (name) => {
      this.greet(name);
    });
    if (result !== true) {
      log("Could not start input: " + str(result));
      return 1;
    }
    return 0;
  }
}
