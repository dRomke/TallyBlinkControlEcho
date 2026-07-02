/*
  WirelessANC — TX gateway.

  ATEM SDI IN -> read tally + camctrl -> LoRa.
  Camctrl: type 0x01 raw ICDATA bundle. Tally: type 0x03 compressed.
*/

#include <BMDSDIControl.h>
#include <RadioLib.h>
#include "BoardConfig.h"
#include "MonitorConfig.h"
#include "MonitorLog.h"
#include "MonitorRuntime.h"
#include "TallyCodec.h"
#include "CamctrlCodec.h"
#include "CamctrlLog.h"
#include "BmdShieldInit.h"
#include "BmdShieldDiag.h"
#include <string.h>

BMD_SDITallyControl_Serial sdiTallyControl;
BMD_SDICameraControl_Serial sdiCameraControl;

LLCC68 lora = new Module(
  LORA_PIN_CS,
  LORA_PIN_DIO1,
  LORA_PIN_RESET,
  LORA_PIN_BUSY);

static byte lastTally[256];
static int  lastTallyLen = -1;

static byte lastCamctrl[256];
static int  lastCamctrlLen = -1;

static unsigned long bootMs = 0;
static unsigned long airtimeUs = 0;
static unsigned long loraTxCount = 0;
static unsigned long lastAirtimeReportMs = 0;

bool loraSendRaw(uint8_t type, const byte* data, int len);

void recordLoRaAirtime(size_t packetLen)
{
  airtimeUs += lora.getTimeOnAir(packetLen);
  loraTxCount++;
}

void pollAirtimeReport()
{
  unsigned long now = millis();
  if (now - lastAirtimeReportMs < AIRTIME_REPORT_MS) {
    return;
  }
  lastAirtimeReportMs = now;

  unsigned long onlineMs = now - bootMs;
  if (onlineMs == 0) {
    return;
  }

  unsigned long airMs = airtimeUs / 1000;
  float pct = (airtimeUs / 1000.0f) / (float)onlineMs * 100.0f;

  Serial.print(F("AIRTIME "));
  Serial.print(pct, 2);
  Serial.print(F("% ("));
  Serial.print(airMs);
  Serial.print(F("ms / "));
  Serial.print(onlineMs);
  Serial.print(F("ms online, "));
  Serial.print(loraTxCount);
  Serial.println(F(" LoRa TX)"));
  Serial.flush();
}

bool loraSendTally(const byte* buffer, int bytesRead)
{
  if (bytesRead <= 0) {
    return false;
  }

  if (lastTallyLen == bytesRead && memcmp(lastTally, buffer, bytesRead) == 0) {
    return false;
  }

  byte compBuf[1 + ((TALLY_SDI_MAX + 3) / 4)];
  int compLen = tallyCompress(buffer, bytesRead, compBuf, sizeof(compBuf));
  if (compLen < 0) {
    return false;
  }

  if (!loraSendRaw(LORA_TYPE_TALLY_COMP, compBuf, compLen)) {
    return false;
  }

  memcpy(lastTally, buffer, bytesRead);
  lastTallyLen = bytesRead;
  monitorLogTallySdi("TX", "FORWARD", "change", buffer, bytesRead);
  return true;
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(BOARD_STARTUP_DELAY_MS);

  Serial.print(F("READY "));
  Serial.print(F(MONITOR_BUILD_ID));
  Serial.println(F(" TX"));

  if (monitorSerialEnabled()) {
    Serial.println(F("SKETCH=WirelessANC_TX"));
    Serial.println(F("Blackmagic SDI Control Shield + LoRa"));
    monitorPrintConfig(F("TX"));
  }

  if (bmdShieldInitOnce(sdiTallyControl, sdiCameraControl)) {
    sdiTallyControl.setOverride(false);
    sdiCameraControl.setOverride(false);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_INACTIVE);

  int state = lora.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF);
  if (monitorSerialEnabled()) {
    Serial.print(F("[LLCC68] Initializing ... "));
    Serial.println(state == RADIOLIB_ERR_NONE ? F("ok") : F("failed"));
  } else if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("ERROR LLCC68 init failed"));
  }

  bootMs = millis();
  lastAirtimeReportMs = bootMs;

  if (monitorSerialEnabled()) {
    monitorPrintConfig(F("TX ready"));
    Serial.flush();
  }
}

void loop()
{
  pollMonitorSerial();

  byte buffer[256];

  if (sdiCameraControl.available()) {
    int icLen = sdiCameraControl.regRead8(kDiagRegICLENGTH);
    int bytesRead = sdiCameraControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      camctrlLogBundle(buffer, bytesRead, icLen);

      if (lastCamctrlLen != bytesRead ||
          memcmp(lastCamctrl, buffer, bytesRead) != 0) {
        if (loraSendRaw(LORA_TYPE_CAMCTRL, buffer, bytesRead)) {
          memcpy(lastCamctrl, buffer, bytesRead);
          lastCamctrlLen = bytesRead;
          if (monitorSerialEnabled()) {
            Serial.println(F("CAMCTRL -> LoRa"));
            Serial.flush();
            monitorLogLoraFrame("TX", "TX", LORA_TYPE_CAMCTRL, buffer, bytesRead);
          }
        }
      }
    } else if (bytesRead < 0) {
      if (monitorSerialEnabled()) {
        Serial.println(F("CAMCTRL buffer too small"));
        Serial.flush();
      }
      sdiCameraControl.flushRead();
    }
  }

  if (sdiTallyControl.available()) {
    int bytesRead = sdiTallyControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      loraSendTally(buffer, bytesRead);
    }
  }

  pollAirtimeReport();
  delay(LOOP_DELAY_MS);
}

bool loraSendRaw(uint8_t type, const byte* data, int len)
{
  if (len <= 0) {
    return false;
  }

  if (len > (int)LORA_MAX_RAW) {
    len = LORA_MAX_RAW;
  }

  byte packet[LORA_MAX_PAYLOAD];
  packet[0] = type;
  memcpy(packet + 1, data, len);

  digitalWrite(LED_BUILTIN, LED_ACTIVE);
  int state = lora.transmit(packet, len + 1);
  digitalWrite(LED_BUILTIN, LED_INACTIVE);
  if (state == RADIOLIB_ERR_NONE) {
    recordLoRaAirtime(len + 1);
    return true;
  }
  return false;
}