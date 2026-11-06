function greet(name: string | null): void {
  if (name === null) {
    log("Input cancelled");
  } else {
    log("Hello, " + name);
  }
}

const result = console.read_line("Name: ", greet);
if (result !== true) {
  log("Could not start input: " + str(result));
}
