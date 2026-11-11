#!/usr/bin/env python3
"""Build and run ESP-IDF integration tests across platforms.

Used by `make integration-test-build` and `make integration-test-run`.
"""

from __future__ import annotations

import argparse
import fnmatch
import os
import shlex
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
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


def select_projects(all_projects: list[Path], selectors: list[str]) -> list[Path]:
    if not selectors:
        return all_projects

    by_name = {project.name: project for project in all_projects}
    selected: list[Path] = []
    seen: set[str] = set()

    for selector in selectors:
        normalized = selector.replace("\\", "/").rstrip("/")
        name = Path(normalized).name
        matches = [
            project for project in all_projects if fnmatch.fnmatchcase(project.name, name)
        ]
        if not matches and name in by_name:
            matches = [by_name[name]]
        if not matches:
            available = ", ".join(project.name for project in all_projects)
            raise RuntimeError(
                f"Unknown integration-test project selector '{selector}'. "
                f"Available: {available}"
            )
        for project in matches:
            if project.name not in seen:
                selected.append(project)
                seen.add(project.name)

    return selected


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


def build_project(project: Path, idf_py: list[str]) -> tuple[Path, int]:
    return project, run([*idf_py, "-C", str(project), "build"])


def cmd_build(projects: list[Path], idf_py: list[str], jobs: int) -> int:
    if jobs <= 1 or len(projects) <= 1:
        for project in projects:
            _, rc = build_project(project, idf_py)
            if rc != 0:
                return rc
        return 0

    failures: list[tuple[Path, int]] = []
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = {
            executor.submit(build_project, project, idf_py): project
            for project in projects
        }
        for future in as_completed(futures):
            project, rc = future.result()
            if rc != 0:
                failures.append((project, rc))

    if failures:
        failures.sort(key=lambda item: item[0].name)
        for project, rc in failures:
            print(
                f"Integration test build failed: {project.name} (exit {rc})",
                file=sys.stderr,
            )
        return failures[0][1]
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
    parser.add_argument(
        "projects",
        nargs="*",
        help=(
            "Optional integration-test project names or selectors "
            "(e.g. test_shell_fs or test_shell*). Defaults to all."
        ),
    )
    parser.add_argument("--idf-py", help="Command used to invoke idf.py (overrides IDF_PY env)")
    parser.add_argument("--python", help="Python executable for pytest (overrides IDF_PYTHON env)")
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=1,
        help="Number of integration-test projects to build in parallel (build only).",
    )
    args = parser.parse_args(argv)

    projects = discover_projects()
    if not projects:
        print("No integration-test projects found", file=sys.stderr)
        return 1

    try:
        projects = select_projects(projects, args.projects)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 1

    if args.jobs < 1:
        print("--jobs must be >= 1", file=sys.stderr)
        return 1

    if args.command == "build":
        try:
            idf_py = resolve_idf_py(args.idf_py)
        except RuntimeError as exc:
            print(str(exc), file=sys.stderr)
            return 1
        return cmd_build(projects, idf_py, args.jobs)

    python = resolve_python(args.python)
    return cmd_run(projects, python)


if __name__ == "__main__":
    raise SystemExit(main())
