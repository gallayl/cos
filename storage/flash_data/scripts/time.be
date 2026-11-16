var __cos_mod_1 = cos_import("@cos/system")
var system = __cos_mod_1.find("default")
class Time
    def run(args)
        var info = system.time_info()
        if info["ok"] != true
            print("time: format error (" + info["error"] + ")")
            return 1
        end
        if size(args) >= 1 && args[0] == "utc"
            print(info["utc"])
            return 0
        end
        if size(args) >= 1 && args[0] == "epoch"
            print(str(info["epoch"]))
            return 0
        end
        var synced = "no"
        if info["synced"] == true
            synced = "yes"
        end
        print("Local:  " + info["local"])
        print("UTC:    " + info["utc"])
        print("Synced: " + synced)
        return 0
    end
end

var __cos_entry = Time()
