#!/usr/bin/env python3
"""Generate the clang compile_commands.json used by clang-tidy.

This mirrors the CI lint flow by running:
  IDF_TOOLCHAIN=clang idf.py -B build/clang reconfigure
"""

import glob
import os
import shlex
import subprocess
import sys

BUILD_DIR = "build/clang"


def run(cmd, env):
    print("+", " ".join(shlex.quote(part) for part in cmd))
    return subprocess.call(cmd, env=env)


def detect_esp_rom_elf_dir(env):
    """Best-effort default for ESP-IDF ROM ELFs when export did not set it."""
    # shortcut: assumes IDF tools layout keeps rom elfs under IDF_TOOLS_PATH/esp-rom-elfs; upgrade if ESP-IDF changes discovery
    if env.get("ESP_ROM_ELF_DIR"):
        return env["ESP_ROM_ELF_DIR"]

    tools_path = env.get("IDF_TOOLS_PATH")
    if not tools_path:
        return None

    base = os.path.join(tools_path, "esp-rom-elfs")
    if not os.path.isdir(base):
        return None

    candidates = []
    try:
        for name in os.listdir(base):
            path = os.path.join(base, name)
            if not os.path.isdir(path):
                continue
            if glob.glob(os.path.join(path, "*.elf")) or glob.glob(os.path.join(path, "*", "*.elf")):
                candidates.append(path)
    except OSError:
        return None

    if not candidates:
        return None
    return sorted(candidates)[-1]


def main():
    idf_path = os.environ.get("IDF_PATH")
    if not idf_path:
        print(
            "IDF_PATH is not set. Source the ESP-IDF environment before preparing the lint database.",
            file=sys.stderr,
        )
        return 1

    idf_py = os.path.join(idf_path, "tools", "idf.py")
    if not os.path.exists(idf_py):
        print(
            f"idf.py not found at {idf_py}. Check that IDF_PATH points to a valid ESP-IDF checkout.",
            file=sys.stderr,
        )
        return 1

    python = os.environ.get("IDF_PYTHON") or sys.executable
    os.makedirs(BUILD_DIR, exist_ok=True)

    env = os.environ.copy()
    env["IDF_TOOLCHAIN"] = "clang"

    if not env.get("ESP_ROM_ELF_DIR"):
        rom_dir = detect_esp_rom_elf_dir(env)
        if rom_dir:
            env["ESP_ROM_ELF_DIR"] = rom_dir
            print(f"ESP_ROM_ELF_DIR not set; using {rom_dir}")

    return run([python, idf_py, "-B", BUILD_DIR, "reconfigure"], env)


if __name__ == "__main__":
    sys.exit(main())
