#pragma once

#include <Arduino.h>
#include "MonitorConfig.h"

static inline void monitorPrintHex(const byte* data, int len)
{
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 < len) {
      Serial.print(' ');
    }
  }
}

static inline void monitorPrintFlag(const __FlashStringHelper* name, int enabled)
{
  Serial.print(F("  "));
  Serial.print(name);
  Serial.println(enabled ? F("=on") : F("=off"));
}

#if MONITOR_STARTUP_BANNER
static inline void monitorPrintConfig()
{
  Serial.println(F("[monitor] active flags:"));
  monitorPrintFlag(F("tally_summary"), MONITOR_TALLY_SUMMARY);
  monitorPrintFlag(F("tally_hex"), MONITOR_TALLY_HEX);
  monitorPrintFlag(F("tally_decode_tx"), MONITOR_TALLY_DECODE_TX);
  monitorPrintFlag(F("camctrl_summary"), MONITOR_CAMCTRL_SUMMARY);
  monitorPrintFlag(F("camctrl_hex"), MONITOR_CAMCTRL_HEX);
  monitorPrintFlag(F("camctrl_skipped"), MONITOR_CAMCTRL_SKIPPED);
  monitorPrintFlag(F("lora_rssi"), MONITOR_LORA_RSSI);
  monitorPrintFlag(F("lora_errors"), MONITOR_LORA_ERRORS);
}
#else
static inline void monitorPrintConfig() {}
#endif

#if MONITOR_TALLY_SUMMARY
static inline void monitorLogTallyTx(int len)
{
  Serial.print(F("TALLY ["));
  Serial.print(len);
  Serial.println(F(" bytes] -> LoRa"));
}

static inline void monitorLogTallyRx(int len, float rssi)
{
  Serial.print(F("LoRa -> SDI TALLY ["));
  Serial.print(len);
  Serial.print(F(" bytes]"));
#if MONITOR_LORA_RSSI
  Serial.print(F(" RSSI "));
  Serial.print(rssi);
  Serial.print(F(" dBm"));
#endif
  Serial.println();
}
#else
static inline void monitorLogTallyTx(int) {}
static inline void monitorLogTallyRx(int, float) {}
#endif

#if MONITOR_TALLY_HEX
static inline void monitorLogTallyHex(const char* tag, const byte* data, int len)
{
  const int bytesPerLine = MONITOR_TALLY_HEX_BYTES_PER_LINE;

  Serial.print(tag);
  Serial.print(F(" TALLY hex ["));
  Serial.print(len);
  Serial.println(F("]:"));

  for (int offset = 0; offset < len; offset += bytesPerLine) {
    int chunk = len - offset;
    if (chunk > bytesPerLine) {
      chunk = bytesPerLine;
    }

    Serial.print(F("  "));
    Serial.print(offset);
    Serial.print(F(": "));
    monitorPrintHex(data + offset, chunk);
    Serial.println();
    Serial.flush();
  }
}
#else
static inline void monitorLogTallyHex(const char*, const byte*, int) {}
#endif

#if MONITOR_TALLY_DECODE_TX
static inline void monitorLogTallyDecodeTx(const byte* data, int len)
{
  bool any = false;

  Serial.println(F("TALLY decode:"));

  for (int cam = 1; cam <= len; cam++) {
    uint8_t flags = data[cam - 1];
    bool program = flags & 0x01;
    bool preview = flags & 0x02;

    if (program || preview) {
      any = true;
      Serial.print(F("  cam "));
      Serial.print(cam);
      if (program) {
        Serial.print(F(" PGM"));
      }
      if (preview) {
        Serial.print(F(" PVW"));
      }
      Serial.println();
    }
  }

  if (!any) {
    Serial.println(F("  (no program/preview flags in payload)"));
  }

  Serial.flush();
}
#else
static inline void monitorLogTallyDecodeTx(const byte*, int) {}
#endif

#if MONITOR_CAMCTRL_SUMMARY
static inline void monitorLogCamctrlTx(int pktLen, const byte* pkt)
{
  Serial.print(F("CAMCTRL pkt ["));
  Serial.print(pktLen);
  Serial.print(F(" bytes] cam "));
  Serial.print(pkt[0]);
  Serial.print(F(" cat "));
  Serial.print(pkt[4]);
  Serial.print(F(" param "));
  Serial.print(pkt[5]);
  Serial.println(F(" -> LoRa"));
}

static inline void monitorLogCamctrlRx(int len, float rssi)
{
  Serial.print(F("LoRa -> SDI CAMCTRL ["));
  Serial.print(len);
  Serial.print(F(" bytes]"));
#if MONITOR_LORA_RSSI
  Serial.print(F(" RSSI "));
  Serial.print(rssi);
  Serial.print(F(" dBm"));
#endif
  Serial.println();
}
#else
static inline void monitorLogCamctrlTx(int, const byte*) {}
static inline void monitorLogCamctrlRx(int, float) {}
#endif

#if MONITOR_CAMCTRL_HEX
static inline void monitorLogCamctrlHex(const char* tag, const byte* data, int len)
{
  Serial.print(tag);
  Serial.print(F(" CAMCTRL hex ["));
  Serial.print(len);
  Serial.print(F("]: "));
  monitorPrintHex(data, len);
  Serial.println();
}
#else
static inline void monitorLogCamctrlHex(const char*, const byte*, int) {}
#endif

#if MONITOR_CAMCTRL_SKIPPED
static inline void monitorLogCamctrlSkipped(int pktLen, const byte* pkt)
{
  Serial.print(F("CAMCTRL skip ["));
  Serial.print(pktLen);
  Serial.print(F(" bytes] cam "));
  Serial.print(pkt[0]);
  Serial.println(F(" (dedup)"));
}
#else
static inline void monitorLogCamctrlSkipped(int, const byte*) {}
#endif

#if MONITOR_LORA_ERRORS
static inline void monitorLogError(const __FlashStringHelper* msg)
{
  Serial.println(msg);
}

static inline void monitorLogErrorCode(const __FlashStringHelper* msg, int code)
{
  Serial.print(msg);
  Serial.println(code);
}
#else
static inline void monitorLogError(const __FlashStringHelper*) {}
static inline void monitorLogErrorCode(const __FlashStringHelper*, int) {}
#endif