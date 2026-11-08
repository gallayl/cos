var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
var result = net.ping("example.com")
if result.ok
    log("Reachable at " + result.address + " (" + str(result.rtt_ms) + " ms)")
else
    log("Ping failed: " + result.error)
end
