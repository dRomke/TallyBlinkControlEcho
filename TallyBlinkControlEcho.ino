/*
  TallyBlinkControlEcho — SDI pass-through gateway with LoRa forward.

  Reads camera control and tally from the BMD 3G-SDI Shield (Serial1) and
  forwards raw payloads over LoRa (LLCC68 via RadioLib).

  LoRa framing: 1-byte type prefix + raw SDI payload
    0x01 = camera control (single BMD command packet)
    0x02 = tally (BMD ITDATA)

  Camera control bundles from the ATEM are split into individual packets
  (4-byte header + payload + 4-byte padding) and only novel packets are sent.
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

static const int CAMCTRL_DB_SIZE    = 32;
static const int CAMCTRL_HDR_LEN    = 4;
static const int CAMCTRL_MAX_PACKET = 255;

// Nano Matter built-in LED is active LOW
static const uint8_t LED_ACTIVE   = LOW;
static const uint8_t LED_INACTIVE = HIGH;

struct CamctrlPacketSlot {
  uint8_t len;
  byte    data[CAMCTRL_MAX_PACKET];
};

static CamctrlPacketSlot camctrlDb[CAMCTRL_DB_SIZE];
static int camctrlDbCount = 0;

static byte lastTally[256];
static int  lastTallyLen = -1;
static uint32_t lastTallyTxMs = 0;
static const uint32_t TALLY_REFRESH_MS = 250;

int camctrlPacketTotalLen(const byte* data, int avail)
{
  if (avail < CAMCTRL_HDR_LEN) {
    return -1;
  }

  uint8_t payloadLen = data[1];
  uint8_t padding = (payloadLen % 4) ? (4 - (payloadLen % 4)) : 0;
  int total = CAMCTRL_HDR_LEN + payloadLen + padding;

  if (total < CAMCTRL_HDR_LEN || total > CAMCTRL_MAX_PACKET || avail < total) {
    return -1;
  }

  return total;
}

bool camctrlDbContains(const byte* data, int len)
{
  for (int i = 0; i < camctrlDbCount; i++) {
    if (camctrlDb[i].len == len && memcmp(camctrlDb[i].data, data, len) == 0) {
      return true;
    }
  }
  return false;
}

void camctrlDbInsert(const byte* data, int len)
{
  int shiftCount = camctrlDbCount < CAMCTRL_DB_SIZE ? camctrlDbCount : CAMCTRL_DB_SIZE - 1;
  for (int i = shiftCount; i > 0; i--) {
    camctrlDb[i] = camctrlDb[i - 1];
  }

  camctrlDb[0].len = len;
  memcpy(camctrlDb[0].data, data, len);

  if (camctrlDbCount < CAMCTRL_DB_SIZE) {
    camctrlDbCount++;
  }
}

bool tallyPayloadChanged(const byte* data, int len)
{
  if (lastTallyLen == len && memcmp(lastTally, data, len) == 0) {
    return false;
  }

  memcpy(lastTally, data, len);
  lastTallyLen = len;
  return true;
}

void processCamctrlBuffer(const byte* buffer, int bytesRead)
{
  int offset = 0;

  while (offset < bytesRead) {
    int pktLen = camctrlPacketTotalLen(buffer + offset, bytesRead - offset);
    if (pktLen < 0) {
      Serial.print("CAMCTRL parse error at offset ");
      Serial.print(offset);
      Serial.print(" in ");
      Serial.print(bytesRead);
      Serial.println(" byte bundle");
      break;
    }

    const byte* pkt = buffer + offset;

    if (!camctrlDbContains(pkt, pktLen)) {
      if (loraSendRaw(LORA_TYPE_CAMCTRL, pkt, pktLen)) {
        camctrlDbInsert(pkt, pktLen);
        Serial.print("CAMCTRL pkt [");
        Serial.print(pktLen);
        Serial.print(" bytes] cam ");
        Serial.print(pkt[0]);
        Serial.print(" cat ");
        Serial.print(pkt[4]);
        Serial.print(" param ");
        Serial.print(pkt[5]);
        Serial.println(" -> LoRa");
      }
    }

    offset += pktLen;
  }
}

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
  digitalWrite(LED_BUILTIN, LED_INACTIVE);

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
    if (bytesRead > 0 && loraSendRaw(LORA_TYPE_TALLY, buffer, bytesRead)) {
      Serial.print("TALLY [");
      Serial.print(bytesRead);
      Serial.println(" bytes] -> LoRa");
    }
  }

  if (sdiCameraControl.available()) {
    int bytesRead = sdiCameraControl.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      processCamctrlBuffer(buffer, bytesRead);
    } else if (bytesRead < 0) {
      Serial.println("CAMCTRL buffer too small, flushing");
      sdiCameraControl.flushRead();
    }
  }
}

bool loraSendRaw(uint8_t type, const byte* data, int len)
{
  if (len <= 0) {
    return false;
  }

  if (type == LORA_TYPE_TALLY) {
    bool changed = tallyPayloadChanged(data, len);
    bool tallyRefresh = (millis() - lastTallyTxMs >= TALLY_REFRESH_MS);
    if (!changed && !tallyRefresh) {
      return false;
    }
    lastTallyTxMs = millis();
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

  digitalWrite(LED_BUILTIN, LED_ACTIVE);
  int state = lora.transmit(packet, len + 1);
  digitalWrite(LED_BUILTIN, LED_INACTIVE);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa TX err ");
    Serial.println(state);
    return false;
  }
  return true;
}