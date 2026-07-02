#pragma once

#include <Arduino.h>
#include <string.h>

static const uint8_t LORA_TYPE_CAMCTRL        = 0x01;
static const uint8_t LORA_TYPE_CAMCTRL_BUNDLE = 0x04;
static const uint8_t LORA_TYPE_CAMCTRL_RAW    = 0x05;
static const uint8_t CAMCTRL_BUNDLE_VERSION   = 1;

struct CamctrlPacketView {
  uint8_t     len;
  const byte* data;
};

static const int CAMCTRL_HDR_BYTES = 4;
static const int CAMCTRL_MAX_SDI_PACKET = 64;

// BMD SDI packet: [dest][payloadLen][cmd][rsvd] + payload + pad to 4 bytes.
// payloadLen counts bytes after the 4-byte header (not including padding).
static inline int camctrlPayloadPadding(uint8_t payloadLen)
{
  return (payloadLen % 4) ? (4 - (payloadLen % 4)) : 0;
}

static inline int camctrlPacketTotalLen(const byte* data, int avail)
{
  if (!data || avail < CAMCTRL_HDR_BYTES) {
    return -1;
  }

  uint8_t payloadLen = data[1];
  int bodyLen = CAMCTRL_HDR_BYTES + payloadLen;
  int padding = camctrlPayloadPadding(payloadLen);
  int total = bodyLen + padding;

  if (total > CAMCTRL_MAX_SDI_PACKET) {
    return -1;
  }

  if (avail >= total) {
    return total;
  }

  // Shield ICLENGTH may omit the final byte of the last packet in a bundle.
  if (avail == total - 1) {
    return avail;
  }

  return -1;
}

// Shield ICLENGTH may omit the final padding byte of the last SDI packet.
// Returns 0 if complete, or 1 if exactly one padding byte is missing.
static inline int camctrlBundleMissingPaddingBytes(const byte* bundle, int bundleLen)
{
  if (!bundle || bundleLen < CAMCTRL_HDR_BYTES) {
    return 0;
  }

  int off = 0;
  while (off < bundleLen) {
    int rem = bundleLen - off;
    if (rem < CAMCTRL_HDR_BYTES) {
      return 0;
    }

    uint8_t payloadLen = bundle[off + 1];
    int bodyLen = CAMCTRL_HDR_BYTES + payloadLen;
    int padding = camctrlPayloadPadding(payloadLen);
    int paddedTotal = bodyLen + padding;

    if (paddedTotal > CAMCTRL_MAX_SDI_PACKET) {
      return 0;
    }

    if (rem > paddedTotal) {
      off += paddedTotal;
      continue;
    }

    if (off + rem == bundleLen && rem == paddedTotal - 1) {
      return 1;
    }

    return 0;
  }

  return 0;
}

static inline int camctrlBundleExpectedLen(const byte* bundle, int bundleLen)
{
  return bundleLen + camctrlBundleMissingPaddingBytes(bundle, bundleLen);
}

static inline bool camctrlBytesAllZero(const byte* data, int len)
{
  for (int i = 0; i < len; i++) {
    if (data[i] != 0) {
      return false;
    }
  }
  return true;
}

static inline int camctrlBundleCountPackets(const byte* bundle, int bundleLen)
{
  if (!bundle || bundleLen <= 0) {
    return 0;
  }

  int off = 0;
  int count = 0;

  while (off < bundleLen) {
    int rem = bundleLen - off;
    if (rem < CAMCTRL_HDR_BYTES) {
      break;
    }

    int pktLen = camctrlPacketTotalLen(bundle + off, rem);
    if (pktLen < 0) {
      break;
    }

    count++;
    off += pktLen;
  }

  return count;
}

typedef void (*CamctrlSdiPacketHandler)(
  const byte* pkt,
  int pktLen,
  int idx,
  int total,
  void* ctx);

// Walk concatenated SDI packets. Stops cleanly on trailing zero padding.
// Returns packet count, or -offset-1 on hard parse error at offset.
static inline int camctrlBundleForEachPacket(
  const byte* bundle,
  int bundleLen,
  CamctrlSdiPacketHandler handler,
  void* ctx)
{
  if (!bundle || bundleLen <= 0) {
    return 0;
  }

  int total = camctrlBundleCountPackets(bundle, bundleLen);
  int off = 0;
  int idx = 0;

  while (off < bundleLen) {
    int rem = bundleLen - off;
    if (rem < CAMCTRL_HDR_BYTES) {
      break;
    }

    int pktLen = camctrlPacketTotalLen(bundle + off, rem);
    if (pktLen < 0) {
      if (camctrlBytesAllZero(bundle + off, rem) || rem <= 3) {
        break;
      }
      return -(off + 1);
    }

    idx++;
    if (handler) {
      handler(bundle + off, pktLen, idx, total, ctx);
    }
    off += pktLen;
  }

  return idx;
}

static inline int camctrlPacketContentLen(const byte* pkt, int totalLen)
{
  if (totalLen < CAMCTRL_HDR_BYTES) {
    return totalLen;
  }

  return CAMCTRL_HDR_BYTES + pkt[1];
}

static inline bool camctrlPacketSameCommand(const byte* a, int lenA, const byte* b, int lenB)
{
  if (lenA < 6 || lenB < 6) {
    return lenA == lenB && memcmp(a, b, lenA) == 0;
  }

  int contentA = camctrlPacketContentLen(a, lenA);
  int contentB = camctrlPacketContentLen(b, lenB);
  if (contentA != contentB) {
    return false;
  }

  return memcmp(a, b, contentA) == 0;
}

// [version:1][count:1][len:1][raw BMD packet] * count
static inline int camctrlBundleEncode(
  const CamctrlPacketView* pkts,
  int pktCount,
  byte* out,
  int outMax,
  int* encodedCount)
{
  if (!pkts || !out || !encodedCount || pktCount <= 0 || outMax < 3) {
    return -1;
  }

  int off = 0;
  out[off++] = CAMCTRL_BUNDLE_VERSION;
  int countPos = off++;
  int count = 0;

  for (int i = 0; i < pktCount; i++) {
    uint8_t len = pkts[i].len;
    if (len == 0) {
      return -1;
    }
    if (off + 1 + len > outMax) {
      break;
    }

    out[off++] = len;
    memcpy(out + off, pkts[i].data, len);
    off += len;
    count++;
  }

  if (count == 0) {
    return -1;
  }

  out[countPos] = (byte)count;
  *encodedCount = count;
  return off;
}

typedef void (*CamctrlBundleHandler)(const byte* pkt, int pktLen, void* ctx);

static inline int camctrlBundlePacketCount(const byte* bundle, int bundleLen)
{
  if (!bundle || bundleLen < 3 || bundle[0] != CAMCTRL_BUNDLE_VERSION) {
    return -1;
  }

  return bundle[1];
}

static inline int camctrlBundleForEach(
  const byte* bundle,
  int bundleLen,
  CamctrlBundleHandler handler,
  void* ctx)
{
  if (!bundle || bundleLen < 3 || bundle[0] != CAMCTRL_BUNDLE_VERSION) {
    return -1;
  }

  int count = bundle[1];
  int off = 2;

  for (int i = 0; i < count; i++) {
    if (off >= bundleLen) {
      return -1;
    }

    int len = bundle[off++];
    if (len <= 0 || off + len > bundleLen) {
      return -1;
    }

    if (handler) {
      handler(bundle + off, len, ctx);
    }

    off += len;
  }

  if (off != bundleLen) {
    return -1;
  }

  return count;
}