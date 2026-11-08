var __cos_mod_1 = cos_import("@cos/fs")
var fs = __cos_mod_1.find("default")
def main(args)
    if args.size() != 0
        print("usage: files")
        return 1
    end
    var textPath = "./sample.txt"
    var jsonPath = "./sample.json"
    var wroteText = fs.write_text(textPath, "hello\n")
    if wroteText != true
        print(str(wroteText))
        return 1
    end
    var text = fs.read_text(textPath)
    if !text.ok
        print(text.error)
        return 1
    end
    print(text.text)
    var wroteJson = fs.write_json(jsonPath, {"label": "cos", "count": 2, "flags": [true, false], "child": {"ok": true}})
    if wroteJson != true
        print(str(wroteJson))
        return 1
    end
    var loaded = fs.read_json(jsonPath)
    if !loaded.ok
        print(loaded.error)
        return 1
    end
    var record = loaded.value
    print(record.label)
    if !fs.exists(textPath) || !fs.is_file(textPath) || fs.is_dir(textPath) || !fs.is_dir("./")
        print("unexpected file kind")
        return 1
    end
    var bytes = fs.size(textPath)
    if !bytes.ok
        print(bytes.error)
        return 1
    end
    print("size " + str(bytes.size))
    var parent = fs.list("./../")
    if !parent.ok
        print(parent.error)
        return 1
    end
    var listing = fs.list("/flash")
    if !listing.ok
        print(listing.error)
        return 1
    end
    return 0
end
