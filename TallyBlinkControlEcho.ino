/*
  TallyBlinkControlEcho — SDI pass-through gateway with LoRa forward.

  Reads camera control and tally from the BMD 3G-SDI Shield (Serial1) and
  forwards raw payloads over LoRa (LLCC68 via RadioLib).

  LoRa framing: 1-byte type prefix + raw SDI payload
    0x01 = camera control (BMD ICDATA)
    0x02 = tally (BMD ITDATA)
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
static const size_t  LORA_MAX_RAW      = LORA_MAX_PAYLOAD - 1;

void setup()
{
  Serial.begin(115200);
  delay(2500);
  Serial.println("Blackmagic Design SDI Control Shield + LoRa");

  sdiTallyControl.begin();
  sdiTallyControl.setOverride(false);
  sdiCameraControl.begin();
  sdiCameraControl.setOverride(false);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

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
  byte buffer[256];

  if (sdiTallyControl.available()) {
    int bytesRead = sdiTallyControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      loraSendRaw(LORA_TYPE_TALLY, buffer, bytesRead);
      Serial.print("TALLY [");
      Serial.print(bytesRead);
      Serial.print(" bytes] -> LoRa");
      Serial.println();
    }
  }

  if (sdiCameraControl.available()) {
    int bytesRead = sdiCameraControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      loraSendRaw(LORA_TYPE_CAMCTRL, buffer, bytesRead);
      Serial.print("CAMCTRL [");
      Serial.print(bytesRead);
      Serial.print(" bytes]: ");
      printHex(buffer, bytesRead);
      Serial.println(" -> LoRa");
    } else if (bytesRead < 0) {
      Serial.println("CAMCTRL buffer too small, flushing");
      sdiCameraControl.flushRead();
    }
  }

  delay(10);
}

void loraSendRaw(uint8_t type, const byte* data, int len)
{
  if (len <= 0) {
    return;
  }

  if (len > (int)LORA_MAX_RAW) {
    Serial.print("LoRa truncate ");
    Serial.print(len);
    Serial.print(" -> ");
    Serial.println(LORA_MAX_RAW);
    len = LORA_MAX_RAW;
  }

  byte packet[LORA_MAX_PAYLOAD];
  packet[0] = type;
  memcpy(packet + 1, data, len);

  int state = lora.transmit(packet, len + 1);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa TX err ");
    Serial.println(state);
  }
}

void printHex(const byte* data, int len)
{
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
}