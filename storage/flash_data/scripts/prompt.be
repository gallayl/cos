var __cos_mod_1 = cos_import("@cos/console")
var console = __cos_mod_1.find("default")
class Prompt
    def greet(name)
        if name == nil
            log("Input cancelled")
        else
            log("Hello, " + name)
        end
    end
    def run(_args)
        var result = console.read_line("Name: ", def (name)
            self.greet(name)
        end)
        if result != true
            log("Could not start input: " + str(result))
            return 1
        end
        return 0
    end
end

var __cos_app = Prompt()
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
