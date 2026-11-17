var __cos_mod_1 = cos_import("@cos/console")
var console = __cos_mod_1.find("default")
class PipeStdin
    def run(_args)
        var input = console.stdin()
        if input == nil
            log("stdin: null")
            return 0
        end
        log("stdin: " + input)
        return 0
    end
end

var __cos_entry = PipeStdin()
