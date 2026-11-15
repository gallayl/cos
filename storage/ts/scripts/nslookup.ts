import net from "@cos/net";

export default class Nslookup implements CosApplication {
  run(args: string[]): number {
    if (args.length < 1 || args.length > 2) {
      print("Usage: nslookup <host> [timeout_ms]");
      return 1;
    }

    let timeout_ms = 1000;
    if (args.length === 2) {
      try {
        const parsed = int(args[1]);
        if (str(parsed) !== args[1] || parsed < 100 || parsed > 3000) {
          print("nslookup: timeout must be 100-3000 ms");
          return 1;
        }
        timeout_ms = parsed;
      } catch {
        print("nslookup: timeout must be 100-3000 ms");
        return 1;
      }
    }

    const result = net.resolve(args[0], timeout_ms);
    if (result["ok"] === true) {
      print("Name: " + args[0]);
      print("Address: " + result["address"]);
      return 0;
    }

    print("nslookup: " + args[0] + " failed (" + result["error"] + ")");
    return 1;
  }
}
