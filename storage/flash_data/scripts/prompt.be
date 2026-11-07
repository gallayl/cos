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
