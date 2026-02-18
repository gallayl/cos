import subprocess
import sys

_OrigPopen = subprocess.Popen


class _PatchedPopen(_OrigPopen):
    """
    Work around incompatibilities between pytest-embedded-qemu and ESP-IDF's
    bundled esptool:

    - (All platforms) esptool bundled with ESP-IDF v5.5.x uses 'merge_bin'
      (underscore) but pytest-embedded-qemu sends 'merge-bin' (hyphen).
    - (Windows only) 'esptool.py' is not a valid executable name on Windows
      because CreateProcess cannot resolve .py files. Rewrite to 'esptool'
      so Windows finds esptool.exe.
    """

    @staticmethod
    def _fix_args(args):
        if isinstance(args, (list, tuple)):
            args = list(args)
            if sys.platform == "win32" and args and args[0] == "esptool.py":
                args[0] = "esptool"
            args = ["merge_bin" if a == "merge-bin" else a for a in args]
        elif isinstance(args, str):
            if sys.platform == "win32" and args.startswith("esptool.py "):
                args = "esptool " + args[len("esptool.py "):]
            args = args.replace(" merge-bin ", " merge_bin ")
        return args

    def __init__(self, args, *a, **kw):
        super().__init__(self._fix_args(args), *a, **kw)


subprocess.Popen = _PatchedPopen
