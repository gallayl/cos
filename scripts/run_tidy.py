#!/usr/bin/env python3
"""Sanitize compile_commands.json and run clang-tidy.

ESP-IDF's Xtensa toolchain emits flags that clang-tidy does not understand.
This script strips those flags from a copy of compile_commands.json before
invoking clang-tidy, and exits non-zero when warnings-as-errors are found.
"""

import json
import os
import subprocess
import sys

UNSUPPORTED_FLAGS = {
    "-mlongcalls",
    "-fno-shrink-wrap",
    "-fno-tree-switch-conversion",
    "-fstrict-volatile-bitfields",
}

SRC_PATH = "build/compile_commands.json"
DST_PATH = "build/tidy/compile_commands.json"


def sanitize_compile_commands(src_path, dst_path):
    with open(src_path, encoding="utf-8") as f:
        commands = json.load(f)

    for entry in commands:
        words = entry["command"].split()
        entry["command"] = " ".join(w for w in words if w not in UNSUPPORTED_FLAGS)

    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    with open(dst_path, "w", encoding="utf-8") as f:
        json.dump(commands, f)


def run_clang_tidy(compile_commands_dir, sources):
    proc = subprocess.Popen(
        ["clang-tidy", "-p", compile_commands_dir] + sources,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    has_errors = False
    for line in proc.stdout:
        sys.stdout.write(line)
        if "warnings treated as errors" in line:
            has_errors = True

    proc.wait()
    return 1 if has_errors else 0


def main():
    sources = sys.argv[1:]
    if not sources:
        print("Usage: run_tidy.py <source files...>", file=sys.stderr)
        return 1

    sanitize_compile_commands(SRC_PATH, DST_PATH)
    return run_clang_tidy("build/tidy", sources)


if __name__ == "__main__":
    sys.exit(main())
