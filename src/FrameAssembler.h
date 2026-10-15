/**
 * @file FrameAssembler.h
 * @brief Streaming UART assembler with header-checksum resync.
 *
 * Locks onto SOF=0x01, checks HEAD_CKSUM as soon as 8 bytes are present, and
 * searches the leftover bytes for the next SOF when the header is invalid.
 * This recovers from boot-time UART noise without waiting on a forged LEN.
 *
 * 流式组帧：满 8 字节就校验帧头，失败则在剩余字节里重新找 SOF。
 *
 * @copyright © 2024, Seeed Studio
 */

#ifndef SEEED_ARDUINO_MMWAVE_FRAME_ASSEMBLER_H
#define SEEED_ARDUINO_MMWAVE_FRAME_ASSEMBLER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "FrameCodec.h"

namespace seeed {
namespace mmwave {

class FrameAssembler {
 public:
  enum class FeedResult {
    NeedMore,    // Still collecting a candidate frame.
    FrameReady,  // takeFrame() returns one complete candidate.
    Resynced,    // Dropped a false lock and continued scanning.
  };

  /**
   * @brief Consume one UART byte.
   * @param byte Next byte from the serial stream.
   * @return NeedMore, FrameReady, or Resynced.
   *
   * 喂入一字节串口数据。
   */
  FeedResult feed(uint8_t byte);

  /**
   * @brief Feed a buffer and collect every completed candidate frame.
   * @param data Bytes to feed.
   * @param len Number of bytes.
   * @param out Completed frames in arrival order.
   * @return Number of frames appended to `out`.
   *
   * 连续喂入一段字节并收集完整候选帧。
   */
  size_t feedBuffer(const uint8_t* data, size_t len,
                    std::vector<std::vector<uint8_t>>& out);

  /**
   * @brief Move the completed candidate out of the assembler.
   * @return The candidate frame bytes (header + payload + data checksum).
   *
   * 取出刚组好的一帧。
   */
  std::vector<uint8_t> takeFrame();

  void reset();

  bool hasFrame() const { return ready_; }
  size_t buffered() const { return buf_.size(); }

 private:
  FeedResult evaluate();

  std::vector<uint8_t> buf_;
  bool in_frame_ = false;
  bool ready_    = false;
};

}  // namespace mmwave
}  // namespace seeed

#endif  // SEEED_ARDUINO_MMWAVE_FRAME_ASSEMBLER_H
