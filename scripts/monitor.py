#!/usr/bin/env python3
"""Bidirectional serial monitor — Enter toggles log display and firmware logging."""

from __future__ import annotations

import argparse
import select
import sys
import termios
import time
import tty

try:
    import serial
except ImportError:
    print("FAIL: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="LoRa gateway serial monitor")
    parser.add_argument("label", help="TX or RX label prefix")
    parser.add_argument("port", help="Serial port path")
    parser.add_argument("baud", nargs="?", type=int, default=115200)
    return parser.parse_args()


def always_show(text: str) -> bool:
    return (
        "[monitor] serial" in text
        or text.startswith("AIRTIME ")
        or text.startswith("READY ")
        or text.startswith("ERROR ")
    )


def main() -> int:
    args = parse_args()
    label = args.label
    port = args.port
    baud = args.baud

    show_logs = False
    old_tty = termios.tcgetattr(sys.stdin)

    def restore_tty() -> None:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_tty)

    print(
        f"[{label}] monitor ready — logging OFF, press Enter to toggle (display + board)",
        flush=True,
    )

    try:
        while True:
            try:
                ser = serial.Serial(port, baud, timeout=0.1)
            except serial.SerialException as exc:
                print(f"[{label}] waiting for {port} ({exc})", flush=True)
                time.sleep(2)
                continue

            print(
                f"[{label}] connected {port} @ {baud} baud — Enter toggles logging",
                flush=True,
            )

            try:
                tty.setcbreak(sys.stdin.fileno())
                while True:
                    ready, _, _ = select.select([ser.fd, sys.stdin], [], [], 0.1)

                    if ser.fd in ready:
                        raw = ser.readline()
                        if not raw:
                            continue
                        text = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                        if show_logs or always_show(text):
                            print(f"[{label}] {text}", flush=True)

                    if sys.stdin in ready:
                        ch = sys.stdin.read(1)
                        if ch not in ("\n", "\r"):
                            continue

                        show_logs = not show_logs
                        ser.write(b"\n")
                        state = "ON" if show_logs else "OFF"
                        print(f"[{label}] *** logging {state} ***", flush=True)
            except serial.SerialException:
                print(f"[{label}] disconnected, retrying in 1s ...", flush=True)
                time.sleep(1)
            finally:
                restore_tty()
                ser.close()
    except KeyboardInterrupt:
        restore_tty()
        print(f"\n[{label}] monitor stopped", flush=True)
        return 0


if __name__ == "__main__":
    sys.exit(main())