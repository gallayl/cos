var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
def main(args)
    if size(args) < 1 || size(args) > 2
        print("Usage: ping <host> [timeout_ms]")
        return 1
    end

    var timeout_ms = 1000
    if size(args) == 2
        try
            var parsed = int(args[1])
            if str(parsed) != args[1] || parsed < 100 || parsed > 3000
                print("ping: timeout must be 100-3000 ms")
                return 1
            end
            timeout_ms = parsed
        except ..
            print("ping: timeout must be 100-3000 ms")
            return 1
        end
    end

    var result = net.ping(args[0], timeout_ms)
    if result["ok"] == true
        print("Reply from " + result["address"] + ": time=" + str(result["rtt_ms"]) + " ms")
        return 0
    end

    print("ping: " + args[0] + " failed (" + result["error"] + ")")
    return 1
end
return cos_module(nil, "main", main)
