import net from "@cos/net";

export function main(args: string[]): int {
  if (size(args) < 1 || size(args) > 2) {
    print("Usage: ping <host> [timeout_ms]");
    return 1;
  }

  var timeout_ms = 1000;
  if (size(args) == 2) {
    try {
      var parsed = int(args[1]);
      if (str(parsed) != args[1] || parsed < 100 || parsed > 3000) {
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
  if (result["ok"]) {
    print("Reply from " + result["address"] + ": time=" + str(result["rtt_ms"]) + " ms");
    return 0;
  }

  print("ping: " + args[0] + " failed (" + result["error"] + ")");
  return 1;
}
