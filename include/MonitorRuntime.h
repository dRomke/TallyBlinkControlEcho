#pragma once

#include <Arduino.h>
#include "MonitorConfig.h"

#if MONITOR_RUNTIME_TOGGLE

static bool g_monitorSerialEnabled = false;

static inline bool monitorSerialEnabled()
{
  return g_monitorSerialEnabled;
}

static inline void monitorSerialSetEnabled(bool enabled)
{
  g_monitorSerialEnabled = enabled;
  Serial.print(F("[monitor] serial "));
  Serial.println(enabled ? F("ON") : F("OFF"));
  Serial.flush();
}

static inline void monitorSerialToggle()
{
  monitorSerialSetEnabled(!g_monitorSerialEnabled);
}

static inline void pollMonitorSerial()
{
  if (!Serial.available()) {
    return;
  }

  bool toggle = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      toggle = true;
    }
  }

  if (toggle) {
    monitorSerialToggle();
  }
}

#define MONITOR_GUARD() \
  do { \
    if (!monitorSerialEnabled()) { \
      return; \
    } \
  } while (0)

#else

static inline void pollMonitorSerial() {}

#define MONITOR_GUARD()

#endif