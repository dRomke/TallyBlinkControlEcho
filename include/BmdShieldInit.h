#pragma once

#include <Arduino.h>
#include <BMDSDIControl.h>
#include "MonitorRuntime.h"

static const int BMD_SHIELD_INIT_ATTEMPTS = 5;

// Both tally and cam objects must call begin() — they share Serial1/shield but
// the original working sketches always initialized each object separately.
inline bool bmdShieldInitOnce(
  BMD_SDITallyControl_Serial& tally,
  BMD_SDICameraControl_Serial& cam)
{
  for (int attempt = 1; attempt <= BMD_SHIELD_INIT_ATTEMPTS; attempt++) {
    if (monitorSerialEnabled()) {
      Serial.print(F("BOOT shield init attempt "));
      Serial.print(attempt);
      Serial.print('/');
      Serial.print(BMD_SHIELD_INIT_ATTEMPTS);
      Serial.print(F(" ... "));
    }

    if (attempt > 1) {
      delay(1000);
    }

    tally.begin();
    cam.begin();

    if (tally.shieldInitialized && cam.shieldInitialized) {
      if (monitorSerialEnabled()) {
        Serial.println(F("ok"));
      }
      return true;
    }

    if (monitorSerialEnabled()) {
      Serial.println(F("timeout"));
    }
  }

  Serial.println(F("BOOT shield init FAILED — reseat shield, check stacking"));
  return false;
}