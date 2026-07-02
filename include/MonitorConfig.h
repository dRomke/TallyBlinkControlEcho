#pragma once

/*
  TX/RX serial logging — edit flags here (include/ copy).
  Mirror of MonitorConfig.h in project root — keep both in sync.
*/

#define MONITOR_BUILD_ID "wirelessanc-v1"

#define MONITOR_LORA_RSSI       1
#define MONITOR_LORA_ERRORS     1
#define MONITOR_STARTUP_BANNER  1
#define MONITOR_RUNTIME_TOGGLE  1

// Legacy flags — kept for Makefile compatibility; packet-log view ignores these.
#define MONITOR_TALLY_SUMMARY       0
#define MONITOR_TALLY_HEX           0
#define MONITOR_TALLY_DECODE_TX     0
#define MONITOR_TALLY_HEX_BYTES_PER_LINE 16
#define MONITOR_TALLY_HEX_ON_REFRESH 0
#define MONITOR_CAMCTRL_SUMMARY     0
#define MONITOR_CAMCTRL_HEX         0
#define MONITOR_CAMCTRL_HEX_BYTES_PER_LINE 16
#define MONITOR_CAMCTRL_SKIPPED     0
#define MONITOR_SHIELD_STATUS       0
#define MONITOR_SDI_WATCHDOG_MS     0