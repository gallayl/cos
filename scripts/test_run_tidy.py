import json
import os
import shlex
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from run_tidy import host_target_triple, run_clang_tidy, sanitize_compile_commands


class SanitizeCompileCommandsTests(unittest.TestCase):
    def test_sanitizes_command_and_arguments_entries(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = os.path.join(temp_dir, "compile_commands.json")
            destination = os.path.join(temp_dir, "tidy", "compile_commands.json")
            entries = [
                {
                    "directory": temp_dir,
                    "command": 'cc -mlongcalls -DNAME=one -c source.c',
                    "file": "source.c",
                },
                {
                    "directory": temp_dir,
                    "arguments": ["cc", "-fno-shrink-wrap", "-I", "include path", "-c", "other.c"],
                    "file": "other.c",
                },
                {
                    "directory": temp_dir,
                    "command": 'cc -mlongcalls -c "C:\\path with spaces\\source.c"',
                    "file": "C:\\path with spaces\\source.c",
                },
            ]
            with open(source, "w", encoding="utf-8") as compile_commands:
                json.dump(entries, compile_commands)

            sanitize_compile_commands(source, destination)

            with open(destination, encoding="utf-8") as compile_commands:
                sanitized = json.load(compile_commands)
            self.assertNotIn("-mlongcalls", sanitized[0]["command"])
            self.assertIn("-DNAME=one", sanitized[0]["command"])
            self.assertEqual(["cc", "-I", "include path", "-c", "other.c"], sanitized[1]["arguments"])
            expected_command = ["cc", "-c", "C:\\path with spaces\\source.c"]
            expected_command = (
                subprocess.list2cmdline(expected_command) if os.name == "nt" else shlex.join(expected_command)
            )
            self.assertEqual(expected_command, sanitized[2]["command"])
            self.assertEqual(temp_dir, sanitized[1]["directory"])


class RunClangTidyTests(unittest.TestCase):
    @patch("run_tidy.subprocess.Popen")
    def test_propagates_clang_tidy_exit_status(self, popen):
        process = popen.return_value
        process.stdout = ["clang-tidy error\n"]
        process.returncode = 7

        self.assertEqual(7, run_clang_tidy("build/tidy", ["source.c"]))
        popen.assert_called_once_with(
            [
                "clang-tidy",
                f"--extra-arg-before=--target={host_target_triple()}",
                "-p",
                "build/tidy",
                "source.c",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

    @patch("run_tidy.subprocess.Popen")
    def test_warnings_as_errors_fail_even_with_zero_exit_status(self, popen):
        process = popen.return_value
        process.stdout = ["3 warnings treated as errors\n"]
        process.returncode = 0

        self.assertEqual(1, run_clang_tidy("build/tidy", ["source.c"]))


if __name__ == "__main__":
    unittest.main()
