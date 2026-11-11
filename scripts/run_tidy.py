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
    "-fno-malloc-dce",
    "-fzero-init-padding-bits=all",
    "-fno-builtin-memcpy",
    "-fno-builtin-memset",
    "-fno-builtin-bzero",
    "-Wno-old-style-declaration",
}

DEFAULT_SRC_PATHS = ("build/clang/compile_commands.json", "build/compile_commands.json")
DST_PATH = "build/tidy/compile_commands.json"


def resolve_compile_commands_path():
    configured = os.environ.get("TIDY_COMPILE_COMMANDS")
    if configured:
        return configured
    for path in DEFAULT_SRC_PATHS:
        if os.path.exists(path):
            return path
    return DEFAULT_SRC_PATHS[-1]


def unescape_windows_arg(arg):
    # shlex.split(posix=False) can preserve escaped quotes in -D...=\"...\" tokens.
    # Convert common escaped forms so compile_commands arguments contain the intended values.
    if '\\"' in arg:
        arg = arg.replace('\\"', '"')
    return arg


def ensure_target_args(command_args):
    if any(arg == "--target" or arg.startswith("--target=") or arg == "-target" for arg in command_args):
        return command_args
    target = os.environ.get("LINT_TARGET_TRIPLE", "xtensa-esp-unknown-elf")
    mcpu = os.environ.get("LINT_MCPU", "esp32")
    insert_at = 1 if command_args else 0
    extras = [f"--target={target}"]
    if mcpu:
        extras.append(f"-mcpu={mcpu}")
    return command_args[:insert_at] + extras + command_args[insert_at:]


def sanitize_compile_commands(src_path, dst_path):
    with open(src_path, encoding="utf-8") as f:
        commands = json.load(f)

    for entry in commands:
        if "arguments" in entry:
            if not isinstance(entry["arguments"], list) or not all(isinstance(arg, str) for arg in entry["arguments"]):
                raise ValueError("compile database arguments must be a list of strings")
            command_args = list(entry["arguments"])
        elif "command" in entry:
            words = shlex.split(entry["command"], posix=os.name != "nt")
            if os.name == "nt":
                words = [
                    word[1:-1] if len(word) >= 2 and word[0] == word[-1] == '"' else word
                    for word in words
                ]
                words = [unescape_windows_arg(word) for word in words]
            command_args = words
        else:
            raise ValueError("compile database entry must contain command or arguments")

        command_args = [
            arg
            for arg in command_args
            if arg not in UNSUPPORTED_FLAGS
            and not arg.startswith("@")
            and not arg.startswith("-specs=")
        ]

        if os.path.splitext(entry.get("file", ""))[1].lower() in {".cc", ".cpp", ".cxx", ".c++"}:
            # clang-tidy cannot parse LovyanGFX's dead throw branch with ESP-IDF's no-exceptions flag.
            command_args = [arg for arg in command_args if arg != "-fno-exceptions"]
            if "-fexceptions" not in command_args:
                command_args.append("-fexceptions")

        command_args = ensure_target_args(command_args)

        if "arguments" in entry or os.name == "nt":
            entry.pop("command", None)
            entry["arguments"] = command_args
        else:
            entry["command"] = shlex.join(command_args)

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
    return proc.returncode if proc.returncode != 0 else (1 if has_errors else 0)


def main():
    sources = sys.argv[1:]
    if not sources:
        print("Usage: run_tidy.py <source files...>", file=sys.stderr)
        return 1

    src = resolve_compile_commands_path()
    if not os.path.exists(src):
        print(f"Compile commands database not found: {src}", file=sys.stderr)
        print("Run 'make lint-db' (in an ESP-IDF/esp-clang environment) or set TIDY_COMPILE_COMMANDS.", file=sys.stderr)
        return 1

    sanitize_compile_commands(src, DST_PATH)
    return run_clang_tidy("build/tidy", sources)


if __name__ == "__main__":
    sys.exit(main())
