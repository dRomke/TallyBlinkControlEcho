#pragma once

#include <Arduino.h>
#include <string.h>

// LoRa tally sub-formats (byte 0 of RF payload, after the type prefix).
static const uint8_t LORA_TYPE_TALLY_RAW  = 0x02;
static const uint8_t LORA_TYPE_TALLY_COMP = 0x03;

static const int TALLY_SDI_MAX = 255;

// [sdiLen:1][packed: ceil(sdiLen/4)] — 2 bits per camera (PGM, PVW).
static inline int tallyCompressedSize(int sdiLen)
{
  if (sdiLen <= 0 || sdiLen > TALLY_SDI_MAX) {
    return -1;
  }
  return 1 + ((sdiLen + 3) / 4);
}

static inline int tallyCompress(const byte* sdi, int sdiLen, byte* out, int outMax)
{
  int compLen = tallyCompressedSize(sdiLen);
  if (compLen < 0 || outMax < compLen) {
    return -1;
  }

  out[0] = (byte)sdiLen;
  memset(out + 1, 0, compLen - 1);

  for (int i = 0; i < sdiLen; i++) {
    uint8_t flags = sdi[i] & 0x03;
    out[1 + (i / 4)] |= (byte)(flags << ((i % 4) * 2));
  }

  return compLen;
}

static inline int tallyDecompress(const byte* comp, int compLen, byte* sdi, int sdiMax)
{
  if (compLen < 2) {
    return -1;
  }

  int sdiLen = comp[0];
  int expected = tallyCompressedSize(sdiLen);
  if (expected < 0 || compLen != expected || sdiLen > sdiMax) {
    return -1;
  }

  memset(sdi, 0, sdiLen);

  for (int i = 0; i < sdiLen; i++) {
    sdi[i] = (comp[1 + (i / 4)] >> ((i % 4) * 2)) & 0x03;
  }

  return sdiLen;
}