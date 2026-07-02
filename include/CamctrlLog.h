#pragma once

#include <Arduino.h>
#include "CamctrlCodec.h"
#include "MonitorRuntime.h"

static inline void camctrlPrintHex(const byte* data, int len)
{
  if (!monitorSerialEnabled()) {
    return;
  }

  for (int i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
  }
}

static inline void camctrlLogPacketLine(
  const byte* pkt,
  int pktLen,
  int idx,
  int total,
  void*)
{
  if (!monitorSerialEnabled()) {
    return;
  }

  Serial.print(F("CAMCTRL pkt "));
  Serial.print(idx);
  Serial.print('/');
  Serial.print(total);
  Serial.print(F(" ["));
  Serial.print(pktLen);
  Serial.print(F("] cam "));
  Serial.print(pkt[0]);
  Serial.print(F(" len "));
  Serial.print(pkt[1]);
  Serial.print(F(" cmd "));
  Serial.print(pkt[2]);
  if (pkt[2] == 0 && pktLen >= 8) {
    Serial.print(F(" cat "));
    Serial.print(pkt[4]);
    Serial.print(F(" param "));
    Serial.print(pkt[5]);
  }
  Serial.print(' ');
  camctrlPrintHex(pkt, pktLen);
  Serial.println();
  Serial.flush();
}

static inline void camctrlLogBundle(
  const byte* bundle,
  int bundleLen,
  int icLen)
{
  if (!monitorSerialEnabled()) {
    return;
  }

  int expectedLen = camctrlBundleExpectedLen(bundle, bundleLen);
  Serial.print(F("CAMCTRL bundle ["));
  Serial.print(bundleLen);
  Serial.print(F(" bytes] iclen="));
  Serial.print(icLen);
  if (expectedLen != bundleLen) {
    Serial.print(F(" expected="));
    Serial.print(expectedLen);
  }
  Serial.println();

  int count = camctrlBundleForEachPacket(
    bundle,
    bundleLen,
    camctrlLogPacketLine,
    nullptr);

  if (count < 0) {
    int errOff = -(count + 1);
    Serial.print(F("CAMCTRL parse error at "));
    Serial.print(errOff);
    Serial.print(F(" rem="));
    int rem = bundleLen - errOff;
    if (rem > 0) {
      Serial.print(' ');
      camctrlPrintHex(bundle + errOff, rem > 16 ? 16 : rem);
    }
    Serial.println();
    Serial.flush();
  }
}