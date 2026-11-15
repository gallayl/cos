import system from "@cos/system";

export default class Uptime implements CosApplication {
  run(_args: string[]): number {
    print("Up " + system.uptime());
    return 0;
  }
}
