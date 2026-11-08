import net from "@cos/net";

const result = net.ping("example.com");
if (result.ok) {
  log("Reachable at " + result.address + " (" + str(result.rtt_ms) + " ms)");
} else {
  log("Ping failed: " + result.error);
}
