var __cos_mod_1 = cos_import("@cos/console")
var console = __cos_mod_1.find("default")
def greet(name)
    if name == nil
        log("Input cancelled")
    else
        log("Hello, " + name)
    end
end
var result = console.read_line("Name: ", greet)
if result != true
    log("Could not start input: " + str(result))
end
