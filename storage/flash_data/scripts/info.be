var __cos_mod_1 = cos_import("@cos/fs")
var fs = __cos_mod_1.find("default")
class Info
    def run(_args)
        var info = fs.storage_info()
        print("Flash (LittleFS):")
        print("  Total: " + str(int(info["flash_total"])) + " bytes")
        print("  Used:  " + str(int(info["flash_used"])) + " bytes")
        if info["sd_mounted"]
            print("SD card (" + info["sd_type"] + "):")
            print("  Total: " + str(int(info["sd_total"])) + " bytes")
            print("  Used:  " + str(int(info["sd_used"])) + " bytes")
        else
            print("SD card: not mounted")
        end
        return 0
    end
end

var __cos_entry = Info()
