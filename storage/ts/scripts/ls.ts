import fs from "@cos/fs";

export default class Ls implements CosApplication {
  run(args: string[]): number {
    let show_all = false;
    let long_format = false;
    let path_arg: string | null = null;
    let i = 0;

    while (i < args.length) {
      let a = args[i];
      if (path_arg !== null && a.length > 0 && (a as any).charAt(0) === "-") {
        return this.usage();
      }
      if (a === "-a") {
        show_all = true;
      } else if (a === "-l") {
        long_format = true;
      } else if (a === "-la" || a === "-al") {
        show_all = true;
        long_format = true;
      } else if (a.length > 0 && (a as any).charAt(0) === "-") {
        return this.usage();
      } else {
        if (path_arg !== null) {
          return this.usage();
        }
        path_arg = a;
      }
      i += 1;
    }

    let cwd = fs.cwd();
    let target = cwd;
    if (path_arg !== null) {
      let resolved = this.normalize_shell_path(path_arg, cwd);
      if (resolved === null) {
        print("ls: invalid path");
        return 1;
      }
      target = resolved;
    } else {
      let resolved_cwd = this.normalize_shell_path(cwd, "/");
      if (resolved_cwd === null) {
        print("ls: invalid path");
        return 1;
      }
      target = resolved_cwd;
    }

    if (target === "/") {
      let root_entries: any[] = [];
      root_entries.push({ name: "flash", size: 0, is_dir: true, mtime: 0 });
      if (fs.exists("/sdcard")) {
        root_entries.push({ name: "sdcard", size: 0, is_dir: true, mtime: 0 });
      }
      return this.print_listing(root_entries, show_all, long_format, false);
    }

    const result: any = fs.list(target);
    if (result["ok"] !== true) {
      print("ls: cannot open '" + target + "'");
      return 1;
    }

    let entries: any[] = [];
    let raw = result["entries"];
    let count = int(result["count"]);
    let j = 0;
    while (j < count) {
      let e: any = raw[j];
      entries.push({
        name: "" + e["name"],
        size: int(e["size"]),
        is_dir: e["is_dir"] === true,
        mtime: int(e["mtime"]),
      });
      j += 1;
    }

    let may_truncate = count === 64;
    return this.print_listing(entries, show_all, long_format, may_truncate);
  }

  usage(): number {
    print("Usage: ls [-a] [-l] [path]");
    return 1;
  }

  normalize_shell_path(input: string, cwd: string): string | null {
    let working = "";
    if (input.length > 0 && (input as any).charAt(0) === "/") {
      working = input;
    } else {
      if (cwd === "/") {
        working = "/" + input;
      } else {
        working = cwd + "/" + input;
      }
    }

    let components: string[] = [];
    let token = "";
    let i = 0;
    while (i <= working.length) {
      let ch = "";
      if (i < working.length) {
        ch = (working as any).charAt(i);
      }
      if (i === working.length || ch === "/") {
        if (token.length === 0) {
          // skip empty component
        } else if (token === ".") {
          // skip
        } else if (token === "..") {
          if (components.length > 0) {
            (components as any).pop();
          }
        } else {
          if (components.length >= 32) {
            return null;
          }
          components.push(token);
        }
        token = "";
      } else {
        token += ch;
      }
      i += 1;
    }

    if (components.length === 0) {
      return "/";
    }

    let result = "";
    let j = 0;
    while (j < components.length) {
      result += "/" + components[j];
      j += 1;
    }
    if (result.length === 0) {
      return "/";
    }
    return result;
  }

  pad_size(file_size: number): string {
    let s = str(int(file_size));
    while (s.length < 5) {
      s = " " + s;
    }
    return s;
  }

  compare_names(a: string, b: string): number {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  }

  sort_entries(entries: any[]): void {
    let i = 1;
    while (i < entries.length) {
      let current = entries[i];
      let j = i - 1;
      while (j >= 0 && this.compare_names(entries[j]["name"], current["name"]) > 0) {
        entries[j + 1] = entries[j];
        j -= 1;
      }
      entries[j + 1] = current;
      i += 1;
    }
  }

  print_listing(entries: any[], show_all: boolean, long_format: boolean, may_truncate: boolean): number {
    let filtered: any[] = [];
    let i = 0;
    while (i < entries.length) {
      let e = entries[i];
      let name = "" + e["name"];
      if (name === "." || name === "..") {
        i += 1;
        continue;
      }
      if (!show_all && name.length > 0 && (name as any).charAt(0) === ".") {
        i += 1;
        continue;
      }
      filtered.push(e);
      i += 1;
    }

    this.sort_entries(filtered);

    let file_count = 0;
    let dir_count = 0;
    let total_bytes = 0;

    if (filtered.length === 0) {
      print("(empty)");
    } else {
      let k = 0;
      while (k < filtered.length) {
        let e = filtered[k];
        let name = "" + e["name"];
        let entry_size = int(e["size"]);
        let mtime = int(e["mtime"]);
        if (e["is_dir"] === true) {
          dir_count += 1;
          if (long_format) {
            print("d " + str(entry_size) + " " + str(mtime) + " " + name + "/");
          } else {
            print("  <DIR>  " + name + "/");
          }
        } else {
          file_count += 1;
          total_bytes += entry_size;
          if (long_format) {
            print("- " + str(entry_size) + " " + str(mtime) + " " + name);
          } else {
            print("  " + this.pad_size(entry_size) + "  " + name);
          }
        }
        k += 1;
      }
    }

    print(str(file_count) + " file(s), " + str(dir_count) + " dir(s), " + str(total_bytes) + " bytes");
    if (may_truncate) {
      print("ls: warning: listing may be truncated at 64 entries");
    }
    return 0;
  }
}
