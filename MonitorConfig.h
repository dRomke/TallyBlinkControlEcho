#pragma once

/*
  TX/RX serial logging — edit flags here (project root copy).
  Mirror of include/MonitorConfig.h — keep both in sync.
*/

#define MONITOR_BUILD_ID "2026-07-03-tally-hex"

#define MONITOR_TALLY_SUMMARY 1
#define MONITOR_TALLY_HEX     1
#define MONITOR_TALLY_DECODE_TX 1
#define MONITOR_TALLY_HEX_BYTES_PER_LINE 16
#define MONITOR_TALLY_HEX_ON_REFRESH 0

#define MONITOR_CAMCTRL_SUMMARY 1
#define MONITOR_CAMCTRL_HEX     1
#define MONITOR_CAMCTRL_HEX_BYTES_PER_LINE 16
#define MONITOR_CAMCTRL_SKIPPED 0

#define MONITOR_LORA_RSSI       1
#define MONITOR_LORA_ERRORS     1
#define MONITOR_STARTUP_BANNER  1