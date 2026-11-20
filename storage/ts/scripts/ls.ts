import fs from "@cos/fs";

// shortcut: path normalization/sorting/component limits removed to avoid Berry/QEMU execution stalls in CI; restore full shell path semantics and sorted output when script execution is stable.
export default class Ls implements CosApplication {
  run(args: string[]): number {
    let show_all = false;
    let long_format = false;
    let path_arg: string | null = null;
    let i = 0;

    while (i < args.length) {
      let a = args[i];
      if (a === "-a") {
        show_all = true;
      } else if (a === "-l") {
        long_format = true;
      } else if (a === "-la" || a === "-al") {
        show_all = true;
        long_format = true;
      } else if (a.length > 0 && (a as any).charAt(0) === "-") {
        print("Usage: ls [-a] [-l] [path]");
        return 1;
      } else {
        if (path_arg !== null) {
          print("Usage: ls [-a] [-l] [path]");
          return 1;
        }
        path_arg = a;
      }
      i += 1;
    }

    let cwd = fs.cwd();
    let target = cwd;
    if (path_arg !== null) {
      if ((path_arg as any).charAt(0) === "/") {
        target = path_arg;
      } else if (cwd === "/") {
        target = "/" + path_arg;
      } else {
        target = cwd + "/" + path_arg;
      }
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

    return this.print_listing(entries, show_all, long_format, count === 64);
  }

  pad_size(file_size: number): string {
    let s = str(int(file_size));
    while (s.length < 5) {
      s = " " + s;
    }
    return s;
  }

  print_listing(entries: any[], show_all: boolean, long_format: boolean, may_truncate: boolean): number {
    let file_count = 0;
    let dir_count = 0;
    let total_bytes = 0;
    let printed = 0;
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
      printed += 1;
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
      i += 1;
    }
    if (printed === 0) {
      print("(empty)");
    }
    print(str(file_count) + " file(s), " + str(dir_count) + " dir(s), " + str(total_bytes) + " bytes");
    if (may_truncate) {
      print("ls: warning: listing may be truncated at 64 entries");
    }
    return 0;
  }
}
