#!/usr/bin/env bash
# Bidirectional serial monitor with auto-reconnect.
# Press Enter to toggle logging (terminal display + firmware serial output).
set -euo pipefail

LABEL="${1:?usage: monitor.sh <label> <port> [baud]}"
PORT="${2:?usage: monitor.sh <label> <port> [baud]}"
BAUD="${3:-115200}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

exec python3 "${SCRIPT_DIR}/monitor.py" "${LABEL}" "${PORT}" "${BAUD}"