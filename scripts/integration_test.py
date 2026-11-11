#!/usr/bin/env python3
"""Build and run ESP-IDF integration tests across platforms.

Used by `make integration-test-build` and `make integration-test-run`.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INTEGRATION_TEST_DIR = ROOT / "integration_test"


def discover_projects() -> list[Path]:
    if not INTEGRATION_TEST_DIR.is_dir():
        return []
    projects = [
        path
        for path in INTEGRATION_TEST_DIR.iterdir()
        if path.is_dir() and path.name.startswith("test_")
    ]
    return sorted(projects, key=lambda p: p.name)


def split_command(value: str) -> list[str]:
    if os.name == "nt":
        # On Windows idf.py/python paths may be passed quoted by Make.
        return shlex.split(value, posix=False)
    return shlex.split(value)


def resolve_idf_py(args_idf_py: str | None) -> list[str]:
    value = args_idf_py or os.environ.get("IDF_PY")
    if value:
        cmd = split_command(value)
        if cmd:
            return cmd

    idf_py = shutil.which("idf.py")
    if idf_py:
        return [idf_py]

    idf_path = os.environ.get("IDF_PATH")
    python = os.environ.get("IDF_PYTHON") or sys.executable
    if idf_path:
        candidate = Path(idf_path) / "tools" / "idf.py"
        if candidate.exists():
            return [python, str(candidate)]

    raise RuntimeError(
        "Could not determine how to invoke idf.py. "
        "Set IDF_PY/IDF_PATH or ensure idf.py is on PATH."
    )


def resolve_python(args_python: str | None) -> str:
    return args_python or os.environ.get("IDF_PYTHON") or sys.executable


def run(cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> int:
    print("+", subprocess.list2cmdline(cmd) if os.name == "nt" else shlex.join(cmd), flush=True)
    completed = subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env)
    return completed.returncode


def find_qemu_bin_dir() -> Path | None:
    exe_name = "qemu-system-xtensa.exe" if os.name == "nt" else "qemu-system-xtensa"
    found = shutil.which(exe_name)
    if found:
        return Path(found).parent

    roots: list[Path] = []
    tools_path = os.environ.get("IDF_TOOLS_PATH")
    if tools_path:
        roots.append(Path(tools_path))
    if os.name == "nt":
        roots.extend([Path(r"C:\Espressif\tools"), Path.home() / ".espressif" / "tools"])
    else:
        roots.append(Path.home() / ".espressif" / "tools")

    seen: set[Path] = set()
    for root in roots:
        try:
            root = root.resolve()
        except OSError:
            continue
        if root in seen or not root.is_dir():
            continue
        seen.add(root)
        for pattern in ("**/qemu/bin/" + exe_name, "**/bin/" + exe_name):
            try:
                for match in root.glob(pattern):
                    if match.is_file():
                        return match.parent
            except OSError:
                continue
    return None


def pytest_env() -> dict[str, str] | None:
    env = os.environ.copy()
    qemu_dir = find_qemu_bin_dir()
    if qemu_dir:
        qemu_str = str(qemu_dir)
        path = env.get("PATH", "")
        parts = path.split(os.pathsep) if path else []
        if qemu_str not in parts:
            env["PATH"] = qemu_str + (os.pathsep + path if path else "")
    return env


def cmd_build(projects: list[Path], idf_py: list[str]) -> int:
    for project in projects:
        rc = run([*idf_py, "-C", str(project), "build"])
        if rc != 0:
            return rc
    return 0


def cmd_run(projects: list[Path], python: str) -> int:
    status = 0
    env = pytest_env()
    for project in projects:
        rc = run(
            [
                python,
                "-m",
                "pytest",
                str(project),
                "--target",
                "esp32",
                "--embedded-services",
                "idf,qemu",
                "-v",
            ],
            env=env,
        )
        if rc != 0:
            status = 1
    return status


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["build", "run"], help="Action to perform")
    parser.add_argument("--idf-py", help="Command used to invoke idf.py (overrides IDF_PY env)")
    parser.add_argument("--python", help="Python executable for pytest (overrides IDF_PYTHON env)")
    args = parser.parse_args(argv)

    projects = discover_projects()
    if not projects:
        print("No integration-test projects found", file=sys.stderr)
        return 1

    if args.command == "build":
        try:
            idf_py = resolve_idf_py(args.idf_py)
        except RuntimeError as exc:
            print(str(exc), file=sys.stderr)
            return 1
        return cmd_build(projects, idf_py)

    python = resolve_python(args.python)
    return cmd_run(projects, python)


if __name__ == "__main__":
    raise SystemExit(main())
