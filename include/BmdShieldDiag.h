#pragma once

#include <Arduino.h>
#include <BMDSDIControl.h>

static const uint16_t kDiagRegICLENGTH = 0x3001;

inline void bmdPrintCamctrlRegs(
  const BMD_SDICameraControl_Serial& cam,
  const BMD_SDITallyControl_Serial& tally)
{
  byte icPeek[4];
  byte itPeek[4];
  int icLen = cam.peekIncoming(icPeek, sizeof(icPeek));
  int itLen = tally.peekIncoming(itPeek, sizeof(itPeek));

  Serial.print(F("SHIELD ic="));
  Serial.print(icLen);
  Serial.print(F(" ic8="));
  Serial.print(cam.regRead8(kDiagRegICLENGTH));
  Serial.print(F(" ic16="));
  Serial.print(cam.regRead16(kDiagRegICLENGTH));
  Serial.print(F(" ic_avail="));
  Serial.print(cam.available() ? '1' : '0');
  Serial.print(F(" it="));
  Serial.print(itLen);
  Serial.print(F(" it_avail="));
  Serial.println(tally.available() ? '1' : '0');
  Serial.flush();
}