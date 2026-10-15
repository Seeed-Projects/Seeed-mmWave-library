/**
 * @file FrameAssembler.cpp
 * @copyright © 2024, Seeed Studio
 */

#include "FrameAssembler.h"

#include <cstddef>

namespace seeed {
namespace mmwave {

namespace {
constexpr size_t kIdxLenHi     = 3;
constexpr size_t kIdxLenLo     = 4;
constexpr size_t kIdxHeadCksum = 7;
}  // namespace

FrameAssembler::FeedResult FrameAssembler::feed(uint8_t byte) {
  if (ready_) {
    buf_.clear();
    ready_    = false;
    in_frame_ = false;
  }

  if (!in_frame_) {
    if (byte != FrameCodec::kSofByte) {
      return FeedResult::NeedMore;
    }
    buf_.clear();
    buf_.push_back(byte);
    in_frame_ = true;
    return FeedResult::NeedMore;
  }

  buf_.push_back(byte);
  return evaluate();
}

size_t FrameAssembler::feedBuffer(const uint8_t* data, size_t len,
                                  std::vector<std::vector<uint8_t>>& out) {
  size_t count = 0;
  for (size_t i = 0; i < len; ++i) {
    if (feed(data[i]) == FeedResult::FrameReady) {
      out.push_back(takeFrame());
      ++count;
    }
  }
  return count;
}

std::vector<uint8_t> FrameAssembler::takeFrame() {
  std::vector<uint8_t> out;
  out.swap(buf_);
  ready_    = false;
  in_frame_ = false;
  return out;
}

void FrameAssembler::reset() {
  buf_.clear();
  in_frame_ = false;
  ready_    = false;
}

FrameAssembler::FeedResult FrameAssembler::evaluate() {
  bool resynced = false;

  while (true) {
    if (buf_.empty()) {
      in_frame_ = false;
      ready_    = false;
      return resynced ? FeedResult::Resynced : FeedResult::NeedMore;
    }

    if (buf_[0] != FrameCodec::kSofByte) {
      size_t sof = 1;
      while (sof < buf_.size() && buf_[sof] != FrameCodec::kSofByte) {
        ++sof;
      }
      if (sof >= buf_.size()) {
        buf_.clear();
        in_frame_ = false;
        ready_    = false;
        return FeedResult::Resynced;
      }
      buf_.erase(buf_.begin(), buf_.begin() + static_cast<std::ptrdiff_t>(sof));
      resynced  = true;
      in_frame_ = true;
      continue;
    }

    if (buf_.size() < FrameCodec::kHeaderSize) {
      in_frame_ = true;
      return resynced ? FeedResult::Resynced : FeedResult::NeedMore;
    }

    const uint16_t data_len = static_cast<uint16_t>(
        (static_cast<uint16_t>(buf_[kIdxLenHi]) << 8) | buf_[kIdxLenLo]);
    const bool header_ok =
        (data_len <= FrameCodec::kMaxDataLen) &&
        (FrameCodec::checksum(buf_.data(), FrameCodec::kHeaderSize - 1) ==
         buf_[kIdxHeadCksum]);

    if (!header_ok) {
      buf_.erase(buf_.begin());
      resynced = true;
      continue;
    }

    const size_t expected =
        FrameCodec::kHeaderSize + data_len + FrameCodec::kDataChecksum;
    if (buf_.size() < expected) {
      in_frame_ = true;
      return resynced ? FeedResult::Resynced : FeedResult::NeedMore;
    }

    if (buf_.size() > expected) {
      buf_.resize(expected);
    }

    ready_    = true;
    in_frame_ = false;
    return FeedResult::FrameReady;
  }
}

}  // namespace mmwave
}  // namespace seeed
