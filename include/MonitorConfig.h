#pragma once

/*
  Serial monitor configuration — edit these flags to enable/disable logging.

  Set to 1 to enable, 0 to disable. Recompile and upload after changes.
*/

// Tally
#define MONITOR_TALLY_SUMMARY 1   // byte-count lines for tally TX/RX
#define MONITOR_TALLY_HEX     1   // hex dump of tally payload (TX and RX)

// Camera control
#define MONITOR_CAMCTRL_SUMMARY 1 // per-packet summary (camera/category/param)
#define MONITOR_CAMCTRL_HEX     0 // hex dump per camera-control packet
#define MONITOR_CAMCTRL_SKIPPED 0 // log camctrl packets suppressed by dedup (TX)

// LoRa
#define MONITOR_LORA_RSSI       1 // RSSI on received packets (RX)
#define MONITOR_LORA_ERRORS     1 // CRC, TX/RX, parse, and startReceive errors

// General
#define MONITOR_STARTUP_BANNER  1 // boot text and active monitor flags