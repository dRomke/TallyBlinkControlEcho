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
#include <string.h>

BMD_SDITallyControl_Serial sdiTallyControl;
BMD_SDICameraControl_Serial sdiCameraControl;

// NSS=10, DIO1=2, NRST=3, BUSY=9
LLCC68 lora = new Module(10, 2, 3, 9);

static const uint8_t LORA_TYPE_CAMCTRL = 0x01;
static const uint8_t LORA_TYPE_TALLY   = 0x02;
static const size_t  LORA_MAX_PAYLOAD  = 255;

void setup()
{
  Serial.begin(115200);
  delay(2500);
  Serial.println("BMD SDI Shield LoRa Receiver");

  sdiTallyControl.begin();
  sdiTallyControl.setOverride(true);
  sdiCameraControl.begin();
  sdiCameraControl.setOverride(true);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print(F("[LLCC68] Initializing ... "));
  int state = lora.begin(863.4, 500, 5);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("ok"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void loop()
{
  byte packet[LORA_MAX_PAYLOAD];

  digitalWrite(LED_BUILTIN, HIGH);
  int state = lora.receive(packet, sizeof(packet));
  digitalWrite(LED_BUILTIN, LOW);

  if (state == RADIOLIB_ERR_NONE) {
    size_t len = lora.getPacketLength();
    if (len < 2) {
      Serial.println("LoRa packet too short");
      return;
    }

    uint8_t type = packet[0];
    const byte* payload = packet + 1;
    int payloadLen = len - 1;

    if (type == LORA_TYPE_CAMCTRL) {
      sdiCameraControl.write(payload, payloadLen, true);
      Serial.print("LoRa -> SDI CAMCTRL [");
      Serial.print(payloadLen);
      Serial.print(" bytes] RSSI ");
      Serial.print(lora.getRSSI());
      Serial.println(" dBm");
    } else if (type == LORA_TYPE_TALLY) {
      sdiTallyControl.write(payload, payloadLen);
      Serial.print("LoRa -> SDI TALLY [");
      Serial.print(payloadLen);
      Serial.print(" bytes] RSSI ");
      Serial.print(lora.getRSSI());
      Serial.println(" dBm");
    } else {
      Serial.print("LoRa unknown type 0x");
      Serial.println(type, HEX);
    }
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("LoRa CRC error");
  } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.print("LoRa RX err ");
    Serial.println(state);
  }
}