/*
  WirelessANC — RX gateway.

  LoRa -> inject tally + camctrl on SDI OUT to camera.
*/

#include <BMDSDIControl.h>
#include <RadioLib.h>
#include "BoardConfig.h"
#include "MonitorConfig.h"
#include "MonitorLog.h"
#include "MonitorRuntime.h"
#include "TallyCodec.h"
#include "CamctrlCodec.h"
#include "BmdShieldInit.h"
#include <string.h>

BMD_SDITallyControl_Serial sdiTallyControl;
BMD_SDICameraControl_Serial sdiCameraControl;

LLCC68 lora = new Module(
  LORA_PIN_CS,
  LORA_PIN_DIO1,
  LORA_PIN_RESET,
  LORA_PIN_BUSY);

static byte lastTallyPayload[256];
static int  lastTallyPayloadLen = 0;
static uint32_t lastTallyInjectMs = 0;

volatile bool loraRxFlag = false;

void onLoraRx()
{
  loraRxFlag = true;
}

void resumeLoRaRx();

void injectTally(const byte* payload, int payloadLen)
{
  if (payloadLen <= 0 || payloadLen > (int)sizeof(lastTallyPayload)) {
    return;
  }

  memcpy(lastTallyPayload, payload, payloadLen);
  lastTallyPayloadLen = payloadLen;
  sdiTallyControl.write(payload, payloadLen);
  lastTallyInjectMs = millis();
}

void handleLoRaPacket(const byte* packet, size_t len)
{
  if (len < 2) {
    return;
  }

  uint8_t type = packet[0];
  const byte* payload = packet + 1;
  int payloadLen = len - 1;

  if (type == LORA_TYPE_TALLY_COMP || type == LORA_TYPE_TALLY_RAW) {
    digitalWrite(LED_BUILTIN, LED_ACTIVE);
    byte sdiBuf[256];
    int sdiLen;

    if (type == LORA_TYPE_TALLY_COMP) {
      sdiLen = tallyDecompress(payload, payloadLen, sdiBuf, sizeof(sdiBuf));
      if (sdiLen < 0) {
        digitalWrite(LED_BUILTIN, LED_INACTIVE);
        return;
      }
    } else {
      memcpy(sdiBuf, payload, payloadLen);
      sdiLen = payloadLen;
    }

    injectTally(sdiBuf, sdiLen);
    digitalWrite(LED_BUILTIN, LED_INACTIVE);
    return;
  }

  if (type == LORA_TYPE_CAMCTRL || type == LORA_TYPE_CAMCTRL_RAW) {
    digitalWrite(LED_BUILTIN, LED_ACTIVE);
    if (monitorSerialEnabled()) {
      Serial.print(F("CAMCTRL inject ["));
      Serial.print(payloadLen);
      Serial.println(F("]"));
      Serial.flush();
    }
    sdiCameraControl.write(payload, payloadLen, true);
    digitalWrite(LED_BUILTIN, LED_INACTIVE);
  }
}

void resumeLoRaRx()
{
  lora.startReceive();
}

void reinjectTallyIfDue()
{
  if (lastTallyPayloadLen <= 0 ||
      millis() - lastTallyInjectMs < TALLY_REINJECT_MS) {
    return;
  }

  resumeLoRaRx();
  sdiTallyControl.write(lastTallyPayload, lastTallyPayloadLen);
  lastTallyInjectMs = millis();
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(BOARD_STARTUP_DELAY_MS);

  Serial.print(F("READY "));
  Serial.print(F(MONITOR_BUILD_ID));
  Serial.println(F(" RX"));

  if (monitorSerialEnabled()) {
    Serial.println(F("SKETCH=WirelessANC_RX"));
    monitorPrintConfig(F("RX"));
  }

  if (bmdShieldInitOnce(sdiTallyControl, sdiCameraControl)) {
    sdiTallyControl.setOverride(true);
    sdiCameraControl.setOverride(true);
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

  lora.setPacketReceivedAction(onLoraRx);
  resumeLoRaRx();

  if (monitorSerialEnabled()) {
    monitorPrintConfig(F("RX ready"));
  }
}

void loop()
{
  pollMonitorSerial();

  while (loraRxFlag) {
    loraRxFlag = false;

    byte packet[LORA_MAX_PAYLOAD];
    int state = lora.readData(packet, sizeof(packet));
    resumeLoRaRx();

    if (state == RADIOLIB_ERR_NONE) {
      handleLoRaPacket(packet, lora.getPacketLength());
    }
  }

  reinjectTallyIfDue();
}