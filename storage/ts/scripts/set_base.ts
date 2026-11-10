import net from "@cos/net";

export default class SetBase implements CosApplication {
  run(args: string[]): number {
    if (args.length !== 1) {
      print("usage: set_base <url>");
      return 1;
    }

    const result = net.set_base_url(args[0]);
    if (result !== true) {
      print(str(result));
      return 1;
    }
    print(net.base_url());
    return 0;
  }
}
