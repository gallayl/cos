function main(args: string[]): number {
  let index = 0;
  while (index < args.length) {
    print(str(index + 1) + ": " + args[index]);
    index += 1;
  }
  return 0;
}
