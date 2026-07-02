# WirelessANC

Wireless SDI gateway for Blackmagic camera control and tally. Two Arduino Nano Matter boards with BMD 3G-SDI Shields and LLCC68 LoRa radios bridge an ATEM switcher to a remote camera without a video return cable.

```
ATEM ──SDI──► [TX] ~~~ LoRa ~~~ [RX] ──SDI──► Camera
```

| Board | Sketch | Role |
|-------|--------|------|
| TX | `WirelessANC_TX` | Read tally + camctrl from SDI IN, forward over LoRa |
| RX | `WirelessANC_RX` | Receive LoRa, inject tally + camctrl on SDI OUT |

Full technical documentation: **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** (also available as [docs/WirelessANC.pdf](docs/WirelessANC.pdf)).

## Hardware

- 2× Arduino Nano Matter (Silicon Labs MG24)
- 2× Blackmagic 3G-SDI Shield (stacked on each Nano)
- 2× LLCC68 LoRa module (863.4 MHz, 500 kHz BW, SF5)
- ATEM (or any SDI tally/camctrl source) on TX SDI IN
- Camera on RX SDI OUT

## Quick start

```bash
# Identify which USB port is TX vs RX
make identify-ports

# Build and flash both boards (OpenOCD adapter-serial targeting)
make upload-both

# Monitor serial (logging off by default — press Enter to toggle)
make monitor-tx    # or monitor-rx / monitor-both
```

Default ports (edit `Makefile` if needed):

- TX: `/dev/cu.usbmodem9582F70C3`
- RX: `/dev/cu.usbmodemE9EB49503`

## Field operation

- **Logging is off by default** on both boards — no packet spam in the field.
- Boot line always prints: `READY wirelessanc-v1 TX` or `RX`.
- **TX only:** `AIRTIME` stats print every 10 seconds regardless of logging state.
- Press **Enter** on the serial port to toggle debug logging (`[monitor] serial ON/OFF`).

## Project layout

```
WirelessANC/
├── WirelessANC_TX/          TX Arduino sketch
├── WirelessANC_RX/          RX Arduino sketch
├── include/                 Shared headers (codecs, logging, board config)
├── libraries/BMDSDIControl/ Blackmagic SDI shield library (patched)
├── scripts/                 Flash, monitor, verify, docs tools
├── docs/                    Architecture documentation
└── Makefile                 Build, flash, monitor targets
```

## Build ID

Current firmware: `wirelessanc-v1` (set in `include/MonitorConfig.h`).