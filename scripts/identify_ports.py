#!/usr/bin/env python3
"""Read serial boot lines to see which port runs TX vs RX sketch."""

import sys
import time

try:
    import serial
except ImportError:
    print("pip install pyserial")
    sys.exit(1)


def sniff(port: str, seconds: float = 4.0) -> list[str]:
    lines: list[str] = []
    try:
        ser = serial.Serial(port, 115200, timeout=0.2)
    except serial.SerialException as exc:
        return [f"ERROR: {exc}"]

    time.sleep(0.3)
    ser.reset_input_buffer()
    end = time.time() + seconds
    while time.time() < end:
        raw = ser.readline()
        if raw:
            lines.append(raw.decode("utf-8", errors="replace").rstrip())
    ser.close()
    return lines


def classify(lines: list[str]) -> str:
    text = "\n".join(lines)
    if "WirelessANC_RX" in text or ("READY" in text and " RX" in text):
        return "RX sketch"
    if "WirelessANC_TX" in text or ("READY" in text and " TX" in text):
        return "TX sketch"
    if "TALLY decode:" in text or "-> LoRa" in text:
        return "likely TX sketch (runtime)"
    if "LoRa -> SDI TALLY" in text:
        return "likely RX sketch (runtime)"
    if "2026-07-03-tally-hex" in text:
        return "new firmware (build id seen)"
    return "unknown (press reset on board while running)"


def main() -> None:
    ports = sys.argv[1:]
    if not ports:
        print("usage: identify_ports.py <port> [<port> ...]")
        sys.exit(1)

    for port in ports:
        print(f"=== {port} ===")
        lines = sniff(port)
        if not lines:
            print("  (no output — press RESET on this board and rerun)")
        else:
            for line in lines[:20]:
                print(f"  {line}")
            if len(lines) > 20:
                print(f"  ... ({len(lines) - 20} more lines)")
        print(f"  => {classify(lines)}")
        print()


if __name__ == "__main__":
    main()