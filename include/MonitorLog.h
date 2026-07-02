#pragma once

#include <Arduino.h>
#include "MonitorConfig.h"
#include "MonitorRuntime.h"

static inline void monitorLogBlankLine()
{
  MONITOR_GUARD();
  Serial.println();
}

static inline void monitorPrintHexLine(const byte* data, int len)
{
  for (int i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

static inline void monitorPrintFlag(const __FlashStringHelper* name, int enabled)
{
  Serial.print(F("  "));
  Serial.print(name);
  Serial.println(enabled ? F("=on") : F("=off"));
}

#if MONITOR_STARTUP_BANNER
static inline void monitorPrintConfig(const __FlashStringHelper* role)
{
  MONITOR_GUARD();
  Serial.println();
  Serial.print(F("[monitor] "));
  Serial.print(F(MONITOR_BUILD_ID));
  Serial.print(F(" "));
  Serial.println(role);
  Serial.println(F("[monitor] Enter toggles logging — packet split view when on"));
  Serial.flush();
}
#else
static inline void monitorPrintConfig(const __FlashStringHelper*) {}
#endif

static inline void monitorLogStreamHeader(
  const char* role,
  const char* stream,
  int bytes,
  int pktCount = -1)
{
  MONITOR_GUARD();
  Serial.println();
  Serial.print(F("--- "));
  Serial.print(role);
  Serial.print(' ');
  Serial.print(stream);
  Serial.print(F(" ["));
  Serial.print(bytes);
  Serial.print(F(" bytes"));
  if (pktCount >= 0) {
    Serial.print(F(", "));
    Serial.print(pktCount);
    Serial.print(F(" pkts"));
  }
  Serial.println(F("] ---"));
  Serial.flush();
}

static inline void monitorLogPacket(
  const char* role,
  int idx,
  int total,
  const char* kind,
  const char* action,
  const char* reason,
  const byte* data,
  int len,
  bool blankBefore = true)
{
  MONITOR_GUARD();
  if (blankBefore && idx > 1) {
    Serial.println();
  }

  Serial.print(role);
  Serial.print(F(" pkt "));
  Serial.print(idx);
  Serial.print('/');
  Serial.print(total);
  Serial.print(' ');
  Serial.print(action);
  Serial.print(' ');
  Serial.print(kind);
  if (reason != nullptr && reason[0] != '\0') {
    Serial.print(F(" ("));
    Serial.print(reason);
    Serial.print(')');
  }
  Serial.print(F(" ["));
  Serial.print(len);
  Serial.println(F(" bytes]"));
  monitorPrintHexLine(data, len);
  Serial.flush();
}

static inline void monitorLogLoraFrame(
  const char* role,
  const char* direction,
  uint8_t type,
  const byte* payload,
  int payloadLen,
  float rssi = 0.0f,
  bool hasRssi = false)
{
  MONITOR_GUARD();
  Serial.println();
  Serial.print(F("--- "));
  Serial.print(role);
  Serial.print(F(" LoRa "));
  Serial.print(direction);
  Serial.print(F(" type=0x"));
  if (type < 0x10) {
    Serial.print('0');
  }
  Serial.print(type, HEX);
  Serial.print(F(" ["));
  Serial.print(payloadLen);
  Serial.print(F(" bytes]"));
#if MONITOR_LORA_RSSI
  if (hasRssi) {
    Serial.print(F(" RSSI "));
    Serial.print(rssi);
    Serial.print(F(" dBm"));
  }
#endif
  Serial.println(F(" ---"));
  monitorPrintHexLine(payload, payloadLen);
  Serial.flush();
}

static inline void monitorLogTallySdi(
  const char* role,
  const char* action,
  const char* reason,
  const byte* data,
  int len)
{
  monitorLogStreamHeader(role, "SDI TALLY", len, 1);
  monitorLogPacket(role, 1, 1, "TALLY", action, reason, data, len, false);
}

static inline void monitorLogTallyRetransmit(
  const char* role,
  const byte* data,
  int len)
{
  monitorLogStreamHeader(role, "SDI TALLY reinject", len, 1);
  monitorLogPacket(role, 1, 1, "TALLY", "RETRANSMIT", "periodic SDI refresh", data, len, false);
}

static inline void monitorLogCamctrlSdiBundle(
  const char* role,
  int bundleLen,
  int totalPkts)
{
  monitorLogStreamHeader(role, "SDI CAMCTRL", bundleLen, totalPkts);
}

static inline void monitorLogCamctrlPacket(
  const char* role,
  int idx,
  int total,
  const char* action,
  const char* reason,
  const byte* pkt,
  int pktLen)
{
  monitorLogPacket(role, idx, total, "CAMCTRL", action, reason, pkt, pktLen, true);
}

static inline void monitorLogLoRaRetry(
  const char* role,
  const char* kind,
  const byte* data,
  int len)
{
  MONITOR_GUARD();
  Serial.println();
  Serial.print(role);
  Serial.print(F(" RETRY LoRa TX "));
  Serial.print(kind);
  if (data != nullptr && len > 0) {
    Serial.print(F(" ["));
    Serial.print(len);
    Serial.println(F(" bytes]"));
    monitorPrintHexLine(data, len);
  } else {
    Serial.println();
  }
  Serial.flush();
}

#if MONITOR_LORA_ERRORS
static inline void monitorLogError(const __FlashStringHelper* msg)
{
  MONITOR_GUARD();
  Serial.print(F("ERROR "));
  Serial.println(msg);
}

static inline void monitorLogErrorCode(const __FlashStringHelper* msg, int code)
{
  MONITOR_GUARD();
  Serial.print(F("ERROR "));
  Serial.print(msg);
  Serial.println(code);
}
#else
static inline void monitorLogError(const __FlashStringHelper*) {}
static inline void monitorLogErrorCode(const __FlashStringHelper*, int) {}
#endif

// Legacy no-ops — callers migrated to monitorLog* above.
static inline void monitorLogTallyTx(int, int) {}
static inline void monitorLogTallyRx(int, float) {}
static inline void monitorLogTallyHex(const char*, const byte*, int) {}
static inline void monitorLogTallyTxData(const char*, const byte*, int) {}
static inline void monitorLogTallyDecodeTx(const byte*, int) {}
static inline void monitorLogCamctrlTx(int, const byte*) {}
static inline void monitorLogCamctrlBundleTx(int, int) {}
static inline void monitorLogCamctrlRx(int, float) {}
static inline void monitorLogCamctrlHex(const char*, const byte*, int) {}
static inline void monitorLogCamctrlSkipped(int, const byte*) {}
static inline void monitorLogShieldStatus(bool, bool, uint16_t, uint16_t, bool) {}
static inline void monitorLogSdiWatchdog(bool, bool) {}