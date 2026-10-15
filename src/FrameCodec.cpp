/**
 * @file FrameCodec.cpp
 * @copyright © 2024, Seeed Studio
 */

#include "FrameCodec.h"

#include <cstring>

namespace seeed {
namespace mmwave {

namespace {
constexpr int kIdxSof       = 0;
constexpr int kIdxIdHi      = 1;
constexpr int kIdxIdLo      = 2;
constexpr int kIdxLenHi     = 3;
constexpr int kIdxLenLo     = 4;
constexpr int kIdxTypeHi    = 5;
constexpr int kIdxTypeLo    = 6;
constexpr int kIdxHeadCksum = 7;

inline bool isBigEndianHost() {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return true;
#else
  return false;
#endif
}

}  // namespace

uint8_t FrameCodec::checksum(const uint8_t* data, size_t len) {
  uint8_t c = 0;
  for (size_t i = 0; i < len; ++i) {
    c ^= data[i];
  }
  return static_cast<uint8_t>(~c);
}

size_t FrameCodec::expectedLength(const uint8_t* buf, size_t len) {
  if (len <= kIdxLenLo) return kMinFrameSize;
  size_t data_len =
      (static_cast<size_t>(buf[kIdxLenHi]) << 8) | buf[kIdxLenLo];
  return kHeaderSize + data_len + kDataChecksum;
}

Status FrameCodec::tryDecode(const uint8_t* buf, size_t len,
                             DecodedFrame& out) {
  if (len < kMinFrameSize) return Status::FrameTooShort;
  if (buf[kIdxSof] != kSofByte) return Status::BadSof;

  const uint16_t id =
      static_cast<uint16_t>((buf[kIdxIdHi] << 8) | buf[kIdxIdLo]);
  const uint16_t data_len =
      static_cast<uint16_t>((buf[kIdxLenHi] << 8) | buf[kIdxLenLo]);
  const uint16_t type =
      static_cast<uint16_t>((buf[kIdxTypeHi] << 8) | buf[kIdxTypeLo]);

  if (data_len > kMaxDataLen) return Status::FrameTooLong;
  if (len < kHeaderSize + data_len + kDataChecksum) return Status::FrameTooShort;

  const uint8_t head_cksum = buf[kIdxHeadCksum];
  const uint8_t data_cksum = buf[kHeaderSize + data_len];

  if (checksum(buf, kHeaderSize - 1) != head_cksum) {
    return Status::ChecksumError;
  }
  if (checksum(buf + kHeaderSize, data_len) != data_cksum) {
    return Status::ChecksumError;
  }

  out.id       = id;
  out.type     = type;
  out.data     = buf + kHeaderSize;
  out.data_len = data_len;
  return Status::Ok;
}

Status FrameCodec::encode(uint16_t id, uint16_t type, const uint8_t* data,
                          size_t data_len, uint8_t* out, size_t out_cap,
                          size_t& out_len) {
  if (data_len > kMaxDataLen) return Status::FrameTooLong;
  const size_t required = kHeaderSize + data_len + kDataChecksum;
  if (out_cap < required) return Status::FrameTooLong;

  out[kIdxSof]       = kSofByte;
  out[kIdxIdHi]      = static_cast<uint8_t>(id >> 8);
  out[kIdxIdLo]      = static_cast<uint8_t>(id & 0xFF);
  out[kIdxLenHi]     = static_cast<uint8_t>(data_len >> 8);
  out[kIdxLenLo]     = static_cast<uint8_t>(data_len & 0xFF);
  out[kIdxTypeHi]    = static_cast<uint8_t>(type >> 8);
  out[kIdxTypeLo]    = static_cast<uint8_t>(type & 0xFF);
  out[kIdxHeadCksum] = checksum(out, kHeaderSize - 1);

  if (data_len > 0 && data != nullptr) {
    std::memcpy(out + kHeaderSize, data, data_len);
  }
  out[kHeaderSize + data_len] = checksum(out + kHeaderSize, data_len);

  out_len = required;
  return Status::Ok;
}

float FrameCodec::readFloatLE(const uint8_t* p) {
  static_assert(sizeof(float) == 4, "unexpected float size");
  float v;
  if (isBigEndianHost()) {
    uint8_t tmp[4] = {p[3], p[2], p[1], p[0]};
    std::memcpy(&v, tmp, 4);
  } else {
    std::memcpy(&v, p, 4);
  }
  return v;
}

uint32_t FrameCodec::readU32LE(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

void FrameCodec::writeFloatLE(float v, uint8_t* p) {
  static_assert(sizeof(float) == 4, "unexpected float size");
  if (isBigEndianHost()) {
    uint8_t tmp[4];
    std::memcpy(tmp, &v, 4);
    p[0] = tmp[3];
    p[1] = tmp[2];
    p[2] = tmp[1];
    p[3] = tmp[0];
  } else {
    std::memcpy(p, &v, 4);
  }
}

void FrameCodec::writeU32LE(uint32_t v, uint8_t* p) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
  p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

}  // namespace mmwave
}  // namespace seeed
