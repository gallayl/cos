import subprocess
import sys

if sys.platform == "win32":
    _OrigPopen = subprocess.Popen

    class _WinPopen(_OrigPopen):
        """
        Work around two Windows/ESP-IDF incompatibilities with pytest-embedded-qemu:
        1. 'esptool.py' is not a valid Windows executable name -> use 'esptool'
        2. Old esptool uses 'merge_bin' (underscore) but plugin sends 'merge-bin' (hyphen)
        """

        @staticmethod
        def _fix_args(args):
            if isinstance(args, (list, tuple)):
                args = list(args)
                if args and args[0] == "esptool.py":
                    args[0] = "esptool"
                args = [a.replace("merge-bin", "merge_bin") if a == "merge-bin" else a for a in args]
            elif isinstance(args, str):
                if args.startswith("esptool.py "):
                    args = "esptool " + args[len("esptool.py "):]
                args = args.replace(" merge-bin ", " merge_bin ")
            return args

        def __init__(self, args, *a, **kw):
            super().__init__(self._fix_args(args), *a, **kw)

    subprocess.Popen = _WinPopen
