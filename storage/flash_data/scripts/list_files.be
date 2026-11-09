var __cos_mod_1 = cos_import("@cos/fs")
var fs = __cos_mod_1.find("default")
class ListFiles
    def run(_args)
        var path = "/flash"
        var result = fs.list(path)
        if result["ok"]
            var entries = result["entries"]
            var index = 0
            while index < result["count"]
                log(entries[index]["name"])
                index += 1
            end
            return 0
        end
        log("Could not list " + path + ": " + result["error"])
        return 1
    end
end

var __cos_app = ListFiles()
def main(args)
    var __cos_r = __cos_app.run(args)
    if __cos_r == nil
        return 0
    end
    return __cos_r
end
if !(_cos_cmd_exec == true)
    main([])
end
