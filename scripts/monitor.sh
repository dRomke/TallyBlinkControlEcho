#!/usr/bin/env bash
# Labeled serial monitor with auto-reconnect.
# Run in a separate Terminal tab; uploads stop the arduino-cli child briefly
# and this script reconnects automatically.
set -euo pipefail

LABEL="${1:?usage: monitor.sh <label> <port> [baud]}"
PORT="${2:?usage: monitor.sh <label> <port> [baud]}"
BAUD="${3:-115200}"

cleanup() {
  if [[ -n "${MONITOR_PID:-}" ]]; then
    kill "${MONITOR_PID}" 2>/dev/null || true
    wait "${MONITOR_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

while true; do
  if [[ ! -e "${PORT}" ]]; then
    printf '[%s] waiting for %s ...\n' "${LABEL}" "${PORT}"
    sleep 2
    continue
  fi

  printf '[%s] connected %s @ %s baud\n' "${LABEL}" "${PORT}" "${BAUD}"
  arduino-cli monitor -p "${PORT}" -c "baudrate=${BAUD}" 2>&1 | sed "s/^/[${LABEL}] /" &
  MONITOR_PID=$!
  wait "${MONITOR_PID}" || true
  unset MONITOR_PID
  printf '[%s] disconnected, retrying in 1s ...\n' "${LABEL}"
  sleep 1
done