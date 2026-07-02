/*
  TallyBlinkControlReceiver — LoRa to SDI inject gateway.

  Receives LoRa packets from TallyBlinkControlEcho (transmitter) and writes
  raw payloads to the BMD 3G-SDI Shield output (Serial1).

  LoRa framing (must match transmitter):
    0x01 + raw BMD camera-control (OCDATA)
    0x02 + raw BMD tally (OTDATA)
*/

#include <BMDSDIControl.h>
#include <RadioLib.h>
#include "MonitorConfig.h"
#include "MonitorLog.h"
#include <string.h>

BMD_SDITallyControl_Serial sdiTallyControl;
BMD_SDICameraControl_Serial sdiCameraControl;

// NSS=10, DIO1=2, NRST=3, BUSY=9
LLCC68 lora = new Module(10, 2, 3, 9);

static const uint8_t LORA_TYPE_CAMCTRL = 0x01;
static const uint8_t LORA_TYPE_TALLY   = 0x02;
static const size_t  LORA_MAX_PAYLOAD  = 255;
static const uint32_t TALLY_REINJECT_MS = 250;

// Nano Matter built-in LED is active LOW
static const uint8_t LED_ACTIVE   = LOW;
static const uint8_t LED_INACTIVE = HIGH;

static byte lastCamctrl[256];
static int  lastCamctrlLen = -1;

static byte lastTallyPayload[256];
static int  lastTallyPayloadLen = 0;
static uint32_t lastTallyInjectMs = 0;

volatile bool loraRxFlag = false;

void onLoraRx()
{
  loraRxFlag = true;
}

bool camctrlChanged(const byte* data, int len)
{
  if (lastCamctrlLen == len && memcmp(lastCamctrl, data, len) == 0) {
    return false;
  }

  memcpy(lastCamctrl, data, len);
  lastCamctrlLen = len;
  return true;
}

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

void reinjectTallyIfDue()
{
  if (lastTallyPayloadLen > 0 &&
      millis() - lastTallyInjectMs >= TALLY_REINJECT_MS) {
    sdiTallyControl.write(lastTallyPayload, lastTallyPayloadLen);
    lastTallyInjectMs = millis();
  }
}

void handleLoRaPacket(const byte* packet, size_t len)
{
  if (len < 2) {
    monitorLogError(F("LoRa packet too short"));
    return;
  }

  uint8_t type = packet[0];
  const byte* payload = packet + 1;
  int payloadLen = len - 1;

  if (type == LORA_TYPE_TALLY) {
    digitalWrite(LED_BUILTIN, LED_ACTIVE);
    float rssi = lora.getRSSI();
    injectTally(payload, payloadLen);
    monitorLogTallyRx(payloadLen, rssi);
    monitorLogTallyHex("RX", payload, payloadLen);
    digitalWrite(LED_BUILTIN, LED_INACTIVE);
    return;
  }

  if (type == LORA_TYPE_CAMCTRL && camctrlChanged(payload, payloadLen)) {
    digitalWrite(LED_BUILTIN, LED_ACTIVE);
    float rssi = lora.getRSSI();
    sdiCameraControl.write(payload, payloadLen, true);
    monitorLogCamctrlRx(payloadLen, rssi);
    monitorLogCamctrlHex("RX", payload, payloadLen);
    digitalWrite(LED_BUILTIN, LED_INACTIVE);
    return;
  }

  if (type != LORA_TYPE_CAMCTRL) {
    monitorLogErrorCode(F("LoRa unknown type 0x"), type);
  }
}

void resumeLoRaRx()
{
  int state = lora.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    monitorLogErrorCode(F("startReceive failed, code "), state);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2500);
  Serial.println(F("SKETCH=TallyBlinkControlReceiver (RX)"));
  Serial.println(F("BMD SDI Shield LoRa Receiver"));
  monitorPrintConfig(F("RX"));

  sdiTallyControl.begin();
  sdiTallyControl.setOverride(true);
  sdiCameraControl.begin();
  sdiCameraControl.setOverride(true);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_INACTIVE);

  Serial.print(F("[LLCC68] Initializing ... "));
  int state = lora.begin(863.4, 500, 5);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("ok"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  lora.setPacketReceivedAction(onLoraRx);
  resumeLoRaRx();
  monitorPrintConfig(F("RX ready"));
}

void loop()
{
  reinjectTallyIfDue();

  while (loraRxFlag) {
    loraRxFlag = false;

    byte packet[LORA_MAX_PAYLOAD];
    int state = lora.readData(packet, sizeof(packet));

    // Resume listening before SDI writes (which can block for frames).
    resumeLoRaRx();

    if (state == RADIOLIB_ERR_NONE) {
      handleLoRaPacket(packet, lora.getPacketLength());
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      monitorLogError(F("LoRa CRC error"));
    } else {
      monitorLogErrorCode(F("LoRa RX err "), state);
    }
  }
}