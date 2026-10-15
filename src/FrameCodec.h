/**
 * @file FrameCodec.h
 * @brief Arduino-free encode/decode of the Seeed mmWave UART frame format.
 *
 * Frame layout:
 *   SOF(1) ID(2) LEN(2) TYPE(2) HEAD_CKSUM(1) DATA(LEN) DATA_CKSUM(1)
 *   SOF = 0x01; ID/LEN/TYPE are big-endian; DATA is little-endian.
 *   HEAD_CKSUM = ~XOR of the seven header bytes that precede it.
 *   DATA_CKSUM = ~XOR of the DATA bytes.
 *
 * 毫米波 UART 帧的纯编解码，不依赖 Arduino，可在主机上单测。
 *
 * @copyright © 2024, Seeed Studio
 */

#ifndef SEEED_ARDUINO_MMWAVE_FRAME_CODEC_H
#define SEEED_ARDUINO_MMWAVE_FRAME_CODEC_H

#include <cstddef>
#include <cstdint>

#include "Status.h"

namespace seeed {
namespace mmwave {

struct DecodedFrame {
  uint16_t id;
  uint16_t type;
  const uint8_t* data;  // Points into the caller-provided buffer.
  uint16_t data_len;
};

struct FrameCodec {
  static constexpr uint8_t kSofByte     = 0x01;
  static constexpr size_t kHeaderSize   = 8;
  static constexpr size_t kDataChecksum = 1;
  static constexpr size_t kMinFrameSize = kHeaderSize + kDataChecksum;
  static constexpr size_t kMaxDataLen   = 512;

  static uint8_t checksum(const uint8_t* data, size_t len);

  /**
   * @brief Project the full frame size from a partial buffer.
   * @param buf First bytes of a frame-in-progress.
   * @param len Number of bytes currently available.
   * @return Total size once LEN is known, otherwise kMinFrameSize.
   *
   * 根据已收到的字节估算完整帧长度。
   */
  static size_t expectedLength(const uint8_t* buf, size_t len);

  /**
   * @brief Decode one complete frame at the start of `buf`.
   * @param buf Candidate frame bytes.
   * @param len Byte count in `buf`.
   * @param out Populated on Status::Ok; `out.data` points into `buf`.
   * @return Ok, FrameTooShort, BadSof, FrameTooLong, or ChecksumError.
   *
   * 解码缓冲区开头的一帧。
   */
  static Status tryDecode(const uint8_t* buf, size_t len, DecodedFrame& out);

  /**
   * @brief Encode a frame into a caller-owned buffer.
   * @param id Frame identifier.
   * @param type Frame type.
   * @param data Payload bytes; may be nullptr when data_len is 0.
   * @param data_len Payload length.
   * @param out Destination buffer.
   * @param out_cap Capacity of `out`.
   * @param out_len Set to the number of bytes written on success.
   * @return Ok or FrameTooLong.
   *
   * 将一帧写入调用方缓冲区。
   */
  static Status encode(uint16_t id, uint16_t type, const uint8_t* data,
                       size_t data_len, uint8_t* out, size_t out_cap,
                       size_t& out_len);

  static float readFloatLE(const uint8_t* p);
  static uint32_t readU32LE(const uint8_t* p);
  static void writeFloatLE(float v, uint8_t* p);
  static void writeU32LE(uint32_t v, uint8_t* p);
};

}  // namespace mmwave
}  // namespace seeed

#endif  // SEEED_ARDUINO_MMWAVE_FRAME_CODEC_H
