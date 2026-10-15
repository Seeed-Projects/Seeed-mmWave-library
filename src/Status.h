/**
 * @file Status.h
 * @brief Decode status codes used by the UART frame codec.
 *
 * 帧编解码状态码，供主机端单元测试和运行时解析共用。
 *
 * @copyright © 2024, Seeed Studio
 */

#ifndef SEEED_ARDUINO_MMWAVE_STATUS_H
#define SEEED_ARDUINO_MMWAVE_STATUS_H

#include <cstdint>

namespace seeed {
namespace mmwave {

enum class Status : uint8_t {
  Ok,               // Frame decoded successfully.
  NoData,           // Requested datum has not arrived since the last read.
  Timeout,          // Deadline elapsed waiting for data.
  ChecksumError,    // Header or data checksum mismatch.
  FrameTooShort,    // Buffer shorter than the required frame size.
  FrameTooLong,     // LEN field exceeds the supported frame buffer.
  BadSof,           // First byte is not the Start-of-Frame marker.
  UnsupportedType,  // Frame type not handled by this device driver.
  Busy,             // Queue is at capacity (oldest frame was applied first).
  IoError,          // Lower-level serial/transport failure.
};

}  // namespace mmwave
}  // namespace seeed

#endif  // SEEED_ARDUINO_MMWAVE_STATUS_H
