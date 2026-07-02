#!/usr/bin/env python3
"""Verify expected firmware strings on a serial port after upload."""

import sys
import time

try:
    import serial
except ImportError:
    print("FAIL: pip install pyserial")
    sys.exit(1)


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: verify_serial.py <port> <expected_substring> [timeout_sec]")
        return 1

    port = sys.argv[1]
    needle = sys.argv[2]
    timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 8.0

    try:
        ser = serial.Serial(port, 115200, timeout=0.2)
    except serial.SerialException as exc:
        print(f"FAIL: cannot open {port}: {exc}")
        return 1

    # Let boot banner arrive; do not flush — upload may have just reset the MCU.
    time.sleep(1.0)
    end = time.time() + timeout
    captured = []
    while time.time() < end:
        line = ser.readline()
        if line:
            text = line.decode("utf-8", errors="replace").rstrip()
            captured.append(text)
            if needle in text:
                print(f"OK: saw '{needle}' on {port}")
                print(f"     {text}")
                ser.close()
                return 0

    ser.close()
    print(f"FAIL: '{needle}' not seen on {port} within {timeout}s")
    if captured:
        print("last lines:")
        for line in captured[-8:]:
            print(f"  {line}")
    else:
        print("hint: press RESET on the board, or check TX_PORT/RX_PORT")
    return 1


if __name__ == "__main__":
    sys.exit(main())