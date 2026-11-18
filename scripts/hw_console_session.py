#!/usr/bin/env python3
"""Run a scripted esp_console session over a serial port and save a transcript.

Intended for on-device validation: sends commands, captures output with
idle/total timeouts, and writes an evidence log. Requires pyserial
(pip install pyserial; available in ESP-IDF Python environments).
"""
from __future__ import annotations

import argparse
import sys
import time
from datetime import datetime
from pathlib import Path


def ts() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]


def log_line(fh, direction: str, text: str) -> None:
    for line in text.splitlines(True):
        fh.write(f"[{ts()}] {direction} {line}")
        if not line.endswith("\n"):
            fh.write("\n")
    fh.flush()


def read_for(ser, idle_s: float, total_s: float) -> str:
    end_total = time.monotonic() + total_s
    end_idle = time.monotonic() + idle_s
    chunks: list[str] = []
    while True:
        now = time.monotonic()
        if now >= end_total or now >= end_idle:
            break
        waiting = getattr(ser, "in_waiting", 0) or 0
        if waiting:
            data = ser.read(waiting)
            if data:
                chunks.append(data.decode("utf-8", errors="replace"))
                end_idle = time.monotonic() + idle_s
                continue
        else:
            # Some backends don't update in_waiting reliably; try a small read.
            data = ser.read(1)
            if data:
                chunks.append(data.decode("utf-8", errors="replace"))
                end_idle = time.monotonic() + idle_s
                continue
        time.sleep(0.02)
    return "".join(chunks)


def load_commands(args: argparse.Namespace) -> list[str]:
    cmds: list[str] = []
    for c in args.cmd or []:
        cmds.append(c)
    if args.command_file:
        for raw in Path(args.command_file).read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            cmds.append(line)
    return cmds


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", required=True, help="Serial port (e.g. COM3, /dev/ttyUSB0)")
    ap.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    ap.add_argument("--cmd", action="append", help="Command to send (repeatable, in order)")
    ap.add_argument("--command-file", help="File with one command per line (# comments allowed)")
    ap.add_argument("--out", required=True, help="Output log file path")
    ap.add_argument("--startup-delay", type=float, default=1.0, help="Seconds to capture initial output before commands")
    ap.add_argument("--idle-ms", type=int, default=400, help="Idle window (ms) after last received byte to consider output complete")
    ap.add_argument("--timeout", type=float, default=10.0, help="Max seconds to wait per command/startup capture")
    ap.add_argument("--reset", action="store_true", help="Toggle DTR/RTS to reset device on open (may reboot device)")
    ap.add_argument("--no-reset", dest="no_reset", action="store_true", help="Deassert reset lines on open (default behavior)")
    args = ap.parse_args()

    cmds = load_commands(args)
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        import serial  # type: ignore
    except Exception as e:  # pragma: no cover
        print(f"error: pyserial is required ({e}). Install in ESP-IDF Python env: python -m pip install pyserial", file=sys.stderr)
        return 2

    idle_s = max(args.idle_ms, 0) / 1000.0
    status = 0
    with out_path.open("w", encoding="utf-8", newline="\n") as fh:
        fh.write(f"# hw_console_session log\n# time={ts()}\n# port={args.port} baud={args.baud}\n# commands={len(cmds)}\n\n")
        try:
            ser = serial.serial_for_url(args.port, args.baud, timeout=0.05, write_timeout=2)
        except Exception as e:
            msg = f"error: failed to open {args.port}: {e}\n"
            fh.write(msg)
            print(msg, file=sys.stderr)
            return 2
        try:
            if args.reset:
                ser.dtr = False
                ser.rts = True
                time.sleep(0.1)
                ser.rts = False
            elif not args.no_reset:
                # Best-effort: avoid unintended reset toggles on some adapters.
                try:
                    ser.dtr = True
                    ser.rts = False
                except Exception:
                    pass
            time.sleep(0.1)
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            if args.startup_delay > 0:
                startup = read_for(ser, idle_s=max(idle_s, 0.2), total_s=max(args.startup_delay, args.timeout))
                if startup:
                    log_line(fh, "RX", startup)

            for i, cmd in enumerate(cmds, 1):
                log_line(fh, "TX", cmd + "\n")
                try:
                    ser.write((cmd + "\n").encode("utf-8"))
                    ser.flush()
                except Exception as e:
                    log_line(fh, "ERR", f"write failed: {e}")
                    print(f"error: command {i} write failed: {e}", file=sys.stderr)
                    status = 1
                    break
                out = read_for(ser, idle_s=idle_s, total_s=args.timeout)
                if out:
                    log_line(fh, "RX", out)
                else:
                    log_line(fh, "INFO", f"(no output within idle={args.idle_ms}ms/timeout={args.timeout}s)")
            # brief tail capture
            tail = read_for(ser, idle_s=min(idle_s, 0.2), total_s=0.3)
            if tail:
                log_line(fh, "RX", tail)
        finally:
            try:
                ser.close()
            except Exception:
                pass
        fh.write(f"\n# end={ts()} status={status}\n")
    print(str(out_path))
    return status


if __name__ == "__main__":
    raise SystemExit(main())
