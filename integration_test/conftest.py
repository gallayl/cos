import subprocess
import sys

_OrigPopen = subprocess.Popen


class _PatchedPopen(_OrigPopen):
    """
    Work around incompatibilities between pytest-embedded-qemu and ESP-IDF's
    bundled esptool:

    - esptool uses 'merge_bin' (underscore) but pytest-embedded-qemu sends
      'merge-bin' (hyphen).
    - (Windows only) 'esptool.py' is not a valid executable name on Windows
      because CreateProcess cannot resolve .py files. Rewrite to 'esptool'
      so Windows finds esptool.exe.
    """

    @staticmethod
    def _fix_args(args):
        if isinstance(args, (list, tuple)):
            args = list(args)
            is_esptool = bool(args) and (
                args[0] in ("esptool.py", "esptool")
                or (args[0] == sys.executable and len(args) >= 3 and args[1] == "-m" and args[2] == "esptool")
            )
            if args and args[0] in ("esptool.py", "esptool"):
                # pytest-embedded may invoke esptool as a bare command, which is
                # not reliably available on Windows. Use the active Python env.
                args = [sys.executable, "-m", "esptool", *args[1:]]
                is_esptool = True
            if is_esptool:
                args = ["merge_bin" if a == "merge-bin" else a for a in args]
            return args

        if isinstance(args, str):
            is_esptool = args.startswith("esptool.py ") or args.startswith("esptool ")
            if args.startswith("esptool.py "):
                rest = args[len("esptool.py "):]
                args = subprocess.list2cmdline([sys.executable, "-m", "esptool"]) + " " + rest
                is_esptool = True
            elif args.startswith("esptool "):
                rest = args[len("esptool "):]
                args = subprocess.list2cmdline([sys.executable, "-m", "esptool"]) + " " + rest
                is_esptool = True
            if is_esptool:
                args = args.replace(" merge-bin ", " merge_bin ")
            return args

        return args

    def __init__(self, args, *a, **kw):
        super().__init__(self._fix_args(args), *a, **kw)


subprocess.Popen = _PatchedPopen
