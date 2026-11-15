var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
class Nslookup
    def run(args)
        if size(args) < 1 || size(args) > 2
            print("Usage: nslookup <host> [timeout_ms]")
            return 1
        end
        var timeout_ms = 1000
        if size(args) == 2
            try
                var parsed = int(args[1])
                if str(parsed) != args[1] || parsed < 100 || parsed > 3000
                    print("nslookup: timeout must be 100-3000 ms")
                    return 1
                end
                timeout_ms = parsed
            except ..
                print("nslookup: timeout must be 100-3000 ms")
                return 1
            end
        end
        var result = net.resolve(args[0], timeout_ms)
        if result["ok"] == true
            print("Name: " + args[0])
            print("Address: " + result["address"])
            return 0
        end
        print("nslookup: " + args[0] + " failed (" + result["error"] + ")")
        return 1
    end
end

var __cos_entry = Nslookup()
