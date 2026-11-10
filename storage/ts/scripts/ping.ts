import net from "@cos/net";

export default class Ping implements CosApplication {
  run(args: string[]): number {
    if (args.length < 1 || args.length > 2) {
      print("Usage: ping <host> [timeout_ms]");
      return 1;
    }

    let timeout_ms = 1000;
    if (args.length === 2) {
      try {
        const parsed = int(args[1]);
        if (str(parsed) !== args[1] || parsed < 100 || parsed > 3000) {
          print("ping: timeout must be 100-3000 ms");
          return 1;
        }
        timeout_ms = parsed;
      } catch {
        print("ping: timeout must be 100-3000 ms");
        return 1;
      }
    }

    const result = net.ping(args[0], timeout_ms);
    if (result["ok"] === true) {
      print("Reply from " + result["address"] + ": time=" + str(result["rtt_ms"]) + " ms");
      return 0;
    }

    print("ping: " + args[0] + " failed (" + result["error"] + ")");
    return 1;
  }
}
