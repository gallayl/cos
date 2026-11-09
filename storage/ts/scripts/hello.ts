export default class Hello implements CosApplication {
  run(_args: string[]): number {
    log("Hello from COS Berry!");
    return 0;
  }
}
