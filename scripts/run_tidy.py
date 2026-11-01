#!/usr/bin/env python3
"""Sanitize compile_commands.json and run clang-tidy.

ESP-IDF's Xtensa toolchain emits flags that clang-tidy does not understand.
This script sanitizes a copy of compile_commands.json before invoking
clang-tidy, and exits non-zero when clang-tidy reports errors or warnings-as-errors.
"""

import json
import os
import shlex
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
        if "arguments" in entry:
            if not isinstance(entry["arguments"], list) or not all(isinstance(arg, str) for arg in entry["arguments"]):
                raise ValueError("compile database arguments must be a list of strings")
            entry["arguments"] = [arg for arg in entry["arguments"] if arg not in UNSUPPORTED_FLAGS]
        elif "command" in entry:
            words = shlex.split(entry["command"], posix=os.name != "nt")
            if os.name == "nt":
                words = [
                    word[1:-1] if len(word) >= 2 and word[0] == word[-1] == '"' else word for word in words
                ]
            words = [word for word in words if word not in UNSUPPORTED_FLAGS]
            entry["command"] = subprocess.list2cmdline(words) if os.name == "nt" else shlex.join(words)
        else:
            raise ValueError("compile database entry must contain command or arguments")

    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    with open(dst_path, "w", encoding="utf-8") as f:
        json.dump(commands, f)


def run_clang_tidy(compile_commands_dir, sources):
    proc = subprocess.Popen(
        [
            "clang-tidy",
            "--query-driver=*xtensa-*-elf-gcc*",
            "-p",
            compile_commands_dir,
        ]
        + sources,
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
    return proc.returncode if proc.returncode != 0 else (1 if has_errors else 0)


def main():
    sources = sys.argv[1:]
    if not sources:
        print("Usage: run_tidy.py <source files...>", file=sys.stderr)
        return 1

    sanitize_compile_commands(SRC_PATH, DST_PATH)
    return run_clang_tidy("build/tidy", sources)


if __name__ == "__main__":
    sys.exit(main())
