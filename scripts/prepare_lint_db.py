#!/usr/bin/env python3
"""Generate the clang compile_commands.json used by clang-tidy.

This mirrors the CI lint flow by running:
  IDF_TOOLCHAIN=clang idf.py -B build/clang reconfigure
"""

import os
import shlex
import subprocess
import sys

BUILD_DIR = "build/clang"


def run(cmd, env):
    print("+", " ".join(shlex.quote(part) for part in cmd))
    return subprocess.call(cmd, env=env)


def main():
    idf_path = os.environ.get("IDF_PATH")
    if not idf_path:
        print("IDF_PATH is not set. Source the ESP-IDF environment before preparing the lint database.", file=sys.stderr)
        return 1

    idf_py = os.path.join(idf_path, "tools", "idf.py")
    if not os.path.exists(idf_py):
        print(f"idf.py not found at {idf_py}. Check that IDF_PATH points to a valid ESP-IDF checkout.", file=sys.stderr)
        return 1

    python = os.environ.get("IDF_PYTHON") or sys.executable
    os.makedirs(BUILD_DIR, exist_ok=True)

    env = os.environ.copy()
    env["IDF_TOOLCHAIN"] = "clang"

    return run([python, idf_py, "-B", BUILD_DIR, "reconfigure"], env)


if __name__ == "__main__":
    sys.exit(main())
