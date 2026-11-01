var path = "/flash"
var result = fs.list(path)

if result["ok"]
    var entries = result["entries"]
    var index = 0
    while index < result["count"]
        log(entries[index]["name"])
        index += 1
    end
else
    log("Could not list " + path + ": " + result["error"])
end
