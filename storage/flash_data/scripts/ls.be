var __cos_mod_1 = cos_import("@cos/fs")
var fs = __cos_mod_1.find("default")
class Ls
    def run(args)
        var show_all = false
        var long_format = false
        var path_arg = nil
        var i = 0
        while i < size(args)
            var a = args[i]
            if a == "-a"
                show_all = true
            elif a == "-l"
                long_format = true
            elif a == "-la" || a == "-al"
                show_all = true
                long_format = true
            elif size(a) > 0 && (a)[0..0] == "-"
                print("Usage: ls [-a] [-l] [path]")
                return 1
            else
                if path_arg != nil
                    print("Usage: ls [-a] [-l] [path]")
                    return 1
                end
                path_arg = a
            end
            i += 1
        end
        var cwd = fs.cwd()
        var target = cwd
        if path_arg != nil
            if (path_arg)[0..0] == "/"
                target = path_arg
            elif cwd == "/"
                target = "/" + path_arg
            else
                target = cwd + "/" + path_arg
            end
        end
        if target == "/"
            var root_entries = []
            root_entries.push({"name": "flash", "size": 0, "is_dir": true, "mtime": 0})
            if fs.exists("/sdcard")
                root_entries.push({"name": "sdcard", "size": 0, "is_dir": true, "mtime": 0})
            end
            return self.print_listing(root_entries, show_all, long_format, false)
        end
        var result = fs.list(target)
        if result["ok"] != true
            print("ls: cannot open '" + target + "'")
            return 1
        end
        var entries = []
        var raw = result["entries"]
        var count = int(result["count"])
        var j = 0
        while j < count
            var e = raw[j]
            entries.push({"name": "" + e["name"], "size": int(e["size"]), "is_dir": e["is_dir"] == true, "mtime": int(e["mtime"])})
            j += 1
        end
        return self.print_listing(entries, show_all, long_format, count == 64)
    end
    def pad_size(file_size)
        var s = str(int(file_size))
        while size(s) < 5
            s = " " + s
        end
        return s
    end
    def print_listing(entries, show_all, long_format, may_truncate)
        var file_count = 0
        var dir_count = 0
        var total_bytes = 0
        var printed = 0
        var i = 0
        while i < size(entries)
            var e = entries[i]
            var name = "" + e["name"]
            if name == "." || name == ".."
                i += 1
                continue
            end
            if !show_all && size(name) > 0 && (name)[0..0] == "."
                i += 1
                continue
            end
            printed += 1
            var entry_size = int(e["size"])
            var mtime = int(e["mtime"])
            if e["is_dir"] == true
                dir_count += 1
                if long_format
                    print("d " + str(entry_size) + " " + str(mtime) + " " + name + "/")
                else
                    print("  <DIR>  " + name + "/")
                end
            else
                file_count += 1
                total_bytes += entry_size
                if long_format
                    print("- " + str(entry_size) + " " + str(mtime) + " " + name)
                else
                    print("  " + self.pad_size(entry_size) + "  " + name)
                end
            end
            i += 1
        end
        if printed == 0
            print("(empty)")
        end
        print(str(file_count) + " file(s), " + str(dir_count) + " dir(s), " + str(total_bytes) + " bytes")
        if may_truncate
            print("ls: warning: listing may be truncated at 64 entries")
        end
        return 0
    end
end

var __cos_entry = Ls()
