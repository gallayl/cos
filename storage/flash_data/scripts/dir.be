var __cos_mod_1 = cos_import("@cos/fs")
var fs = __cos_mod_1.find("default")
class Dir
    def run(args)
        var show_all = false
        var long_format = false
        var path_arg = nil
        var i = 0
        while i < size(args)
            var a = args[i]
            if path_arg != nil && size(a) > 0 && (a)[0..0] == "-"
                return self.usage()
            end
            if a == "-a"
                show_all = true
            elif a == "-l"
                long_format = true
            elif a == "-la" || a == "-al"
                show_all = true
                long_format = true
            elif size(a) > 0 && (a)[0..0] == "-"
                return self.usage()
            else
                if path_arg != nil
                    return self.usage()
                end
                path_arg = a
            end
            i += 1
        end
        var cwd = fs.cwd()
        var target = cwd
        if path_arg != nil
            var resolved = self.normalize_shell_path(path_arg, cwd)
            if resolved == nil
                print("dir: invalid path")
                return 1
            end
            target = resolved
        else
            var resolved_cwd = self.normalize_shell_path(cwd, "/")
            if resolved_cwd == nil
                print("dir: invalid path")
                return 1
            end
            target = resolved_cwd
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
            print("dir: cannot open '" + target + "'")
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
        var may_truncate = count == 64
        return self.print_listing(entries, show_all, long_format, may_truncate)
    end
    def usage()
        print("Usage: dir [-a] [-l] [path]")
        return 1
    end
    def normalize_shell_path(input, cwd)
        var working = ""
        if size(input) > 0 && (input)[0..0] == "/"
            working = input
        else
            if cwd == "/"
                working = "/" + input
            else
                working = cwd + "/" + input
            end
        end
        var components = []
        var token = ""
        var i = 0
        while i <= size(working)
            var ch = ""
            if i < size(working)
                ch = (working)[i..i]
            end
            if i == size(working) || ch == "/"
                if size(token) == 0
                elif token == "."
                elif token == ".."
                    if size(components) > 0
                        (components).pop()
                    end
                else
                    if size(components) >= 32
                        return nil
                    end
                    components.push(token)
                end
                token = ""
            else
                token += ch
            end
            i += 1
        end
        if size(components) == 0
            return "/"
        end
        var result = ""
        var j = 0
        while j < size(components)
            result += "/" + components[j]
            j += 1
        end
        if size(result) == 0
            return "/"
        end
        return result
    end
    def pad_size(file_size)
        var s = str(int(file_size))
        while size(s) < 5
            s = " " + s
        end
        return s
    end
    def compare_names(a, b)
        if a < b
            return -1
        end
        if a > b
            return 1
        end
        return 0
    end
    def sort_entries(entries)
        var i = 1
        while i < size(entries)
            var current = entries[i]
            var j = i - 1
            while j >= 0 && self.compare_names(entries[j]["name"], current["name"]) > 0
                entries[j + 1] = entries[j]
                j -= 1
            end
            entries[j + 1] = current
            i += 1
        end
    end
    def print_listing(entries, show_all, long_format, may_truncate)
        var filtered = []
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
            filtered.push(e)
            i += 1
        end
        self.sort_entries(filtered)
        var file_count = 0
        var dir_count = 0
        var total_bytes = 0
        if size(filtered) == 0
            print("(empty)")
        else
            var k = 0
            while k < size(filtered)
                var e = filtered[k]
                var name = "" + e["name"]
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
                k += 1
            end
        end
        print(str(file_count) + " file(s), " + str(dir_count) + " dir(s), " + str(total_bytes) + " bytes")
        if may_truncate
            print("dir: warning: listing may be truncated at 64 entries")
        end
        return 0
    end
end

var __cos_entry = Dir()
