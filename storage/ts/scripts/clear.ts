// Clear the text console.
export default class Clear implements CosApplication {
  run(_args: string[]): number {
    action("clear");
    return 0;
  }
}
