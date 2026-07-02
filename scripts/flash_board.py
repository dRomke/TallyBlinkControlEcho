#!/usr/bin/env python3
"""Flash a Nano Matter board by USB port using OpenOCD adapter serial filtering.

arduino-cli upload always picks the first CMSIS-DAP adapter when two boards
are connected. This script maps /dev/cu.usbmodem* to adapter serial and
programs the correct physical board.
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

OPENOCD = Path.home() / "Library/Arduino15/packages/SiliconLabs/tools/openocd/0.12.0-arduino1-static/bin/openocd"
SCRIPTS = Path.home() / "Library/Arduino15/packages/SiliconLabs/tools/openocd/0.12.0-arduino1-static/share/openocd/scripts"


def board_serial_for_port(port: str) -> str:
    proc = subprocess.run(
        ["arduino-cli", "board", "list", "--json"],
        check=True,
        capture_output=True,
        text=True,
    )
    data = json.loads(proc.stdout)
    for entry in data.get("detected_ports", []):
        entry_port = entry.get("port", {})
        if entry_port.get("address") == port:
            props = entry_port.get("properties", {})
            serial = props.get("serialNumber") or entry_port.get("hardware_id")
            if serial:
                return serial
    raise RuntimeError(f"no CMSIS-DAP serial found for port {port}")


def flash_hex(port: str, hex_path: Path) -> None:
    if not hex_path.is_file():
        raise FileNotFoundError(hex_path)
    if not OPENOCD.is_file():
        raise FileNotFoundError(OPENOCD)

    adapter_serial = board_serial_for_port(port)
    hex_abs = str(hex_path.resolve())
    commands = (
        f"adapter serial {adapter_serial}; "
        f"init; reset_config srst_nogate; reset halt; "
        f"program {hex_abs}; reset; exit"
    )

    print(f"FLASH {hex_path.name} -> {port} (adapter serial {adapter_serial})")
    subprocess.run(
        [
            str(OPENOCD),
            "-s",
            str(SCRIPTS),
            "-f",
            "interface/cmsis-dap.cfg",
            "-f",
            "target/efm32s2_g23.cfg",
            "-c",
            commands,
        ],
        check=True,
    )


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: flash_board.py <port> <hex-file>")
        return 1

    try:
        flash_hex(sys.argv[1], Path(sys.argv[2]))
    except subprocess.CalledProcessError as exc:
        print(f"FAIL: openocd exited {exc.returncode}", file=sys.stderr)
        return exc.returncode or 1
    except Exception as exc:  # noqa: BLE001 - surface flash errors to make
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())