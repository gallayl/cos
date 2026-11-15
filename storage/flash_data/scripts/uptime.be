var __cos_mod_1 = cos_import("@cos/system")
var system = __cos_mod_1.find("default")
class Uptime
    def run(_args)
        print("Up " + system.uptime())
        return 0
    end
end

var __cos_entry = Uptime()
