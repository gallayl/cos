var __cos_mod_1 = cos_import("@cos/light")
var light = __cos_mod_1.find("default")
class Light
    def run(_args)
        var result = light.read()
        if result["ok"] == true
            print("Light sensor: " + str(result["value"]))
            return 0
        end
        print("light: read failed")
        return 1
    end
end

var __cos_entry = Light()
