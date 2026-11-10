var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
class SetBase
    def run(args)
        if size(args) != 1
            print("usage: set_base <url>")
            return 1
        end
        var result = net.set_base_url(args[0])
        if result != true
            print(str(result))
            return 1
        end
        print(net.base_url())
        return 0
    end
end

var __cos_entry = SetBase()
