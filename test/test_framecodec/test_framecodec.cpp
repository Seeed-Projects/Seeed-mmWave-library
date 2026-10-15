// Host-side unit tests for FrameCodec and FrameAssembler.
//
// Run with `pio test -e native`.

#include <unity.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "FrameAssembler.h"
#include "FrameCodec.h"

using seeed::mmwave::DecodedFrame;
using seeed::mmwave::FrameAssembler;
using seeed::mmwave::FrameCodec;
using seeed::mmwave::Status;

static std::vector<uint8_t> makeFrame(uint16_t id, uint16_t type,
                                      const std::vector<uint8_t>& data) {
  std::vector<uint8_t> buf(FrameCodec::kHeaderSize + data.size() +
                           FrameCodec::kDataChecksum);
  size_t written = 0;
  Status st = FrameCodec::encode(id, type, data.data(), data.size(), buf.data(),
                                 buf.size(), written);
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok), static_cast<int>(st));
  TEST_ASSERT_EQUAL_size_t(buf.size(), written);
  return buf;
}

void setUp(void) {}
void tearDown(void) {}

void test_checksum_is_xor_then_invert(void) {
  const uint8_t a[] = {0x01, 0x02, 0x03};
  TEST_ASSERT_EQUAL_HEX8(0xFF, FrameCodec::checksum(a, sizeof(a)));
  TEST_ASSERT_EQUAL_HEX8(0xFF, FrameCodec::checksum(nullptr, 0));
}

void test_encode_layout_is_stable(void) {
  const std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
  auto frame = makeFrame(/*id=*/0x1234, /*type=*/0x0105, payload);

  TEST_ASSERT_EQUAL_HEX8(FrameCodec::kSofByte, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x12, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x34, frame[2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[3]);
  TEST_ASSERT_EQUAL_HEX8(0x04, frame[4]);
  TEST_ASSERT_EQUAL_HEX8(0x01, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(0x05, frame[6]);
  TEST_ASSERT_EQUAL_HEX8(FrameCodec::checksum(frame.data(), 7), frame[7]);
  TEST_ASSERT_EQUAL_HEX8(0xDE, frame[8]);
  TEST_ASSERT_EQUAL_HEX8(FrameCodec::checksum(frame.data() + 8, 4), frame[12]);
}

void test_encode_decode_roundtrip(void) {
  const std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 6, 7, 8};
  auto frame = makeFrame(0xBEEF, 0x0200, payload);

  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
  TEST_ASSERT_EQUAL_UINT16(0xBEEF, out.id);
  TEST_ASSERT_EQUAL_UINT16(0x0200, out.type);
  TEST_ASSERT_EQUAL_UINT16(payload.size(), out.data_len);
  TEST_ASSERT_EQUAL_MEMORY(payload.data(), out.data, payload.size());
}

void test_decode_rejects_bad_sof(void) {
  auto frame = makeFrame(0x0001, 0x0001, {0xAA});
  frame[0] = 0x02;
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::BadSof),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
}

void test_decode_rejects_short_buffer(void) {
  auto frame = makeFrame(0x0001, 0x0001, {0xAA, 0xBB});
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::FrameTooShort),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size() - 1, out)));
  TEST_ASSERT_EQUAL(static_cast<int>(Status::FrameTooShort),
                    static_cast<int>(
                        FrameCodec::tryDecode(frame.data(), 3, out)));
}

void test_decode_rejects_header_checksum_mismatch(void) {
  auto frame = makeFrame(0x0001, 0x0001, {0xAA});
  frame[7] ^= 0xFF;
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::ChecksumError),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
}

void test_decode_rejects_data_checksum_mismatch(void) {
  auto frame = makeFrame(0x0001, 0x0001, {0xAA, 0xBB});
  frame.back() ^= 0xFF;
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::ChecksumError),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
}

void test_encode_rejects_oversized_payload(void) {
  std::vector<uint8_t> payload(FrameCodec::kMaxDataLen + 1, 0x00);
  std::vector<uint8_t> buf(1024);
  size_t written = 0;
  TEST_ASSERT_EQUAL(static_cast<int>(Status::FrameTooLong),
                    static_cast<int>(FrameCodec::encode(
                        1, 2, payload.data(), payload.size(), buf.data(),
                        buf.size(), written)));
}

void test_encode_rejects_tiny_output_buffer(void) {
  uint8_t out[4];
  size_t written = 0;
  TEST_ASSERT_EQUAL(static_cast<int>(Status::FrameTooLong),
                    static_cast<int>(FrameCodec::encode(1, 2, nullptr, 0, out,
                                                        sizeof(out), written)));
}

void test_expected_length_from_partial_header(void) {
  const uint8_t partial_header[] = {0x01, 0x00, 0x00, 0x00, 0x08};
  TEST_ASSERT_EQUAL_size_t(FrameCodec::kHeaderSize + 8 +
                               FrameCodec::kDataChecksum,
                           FrameCodec::expectedLength(partial_header, 5));
  TEST_ASSERT_EQUAL_size_t(FrameCodec::kMinFrameSize,
                           FrameCodec::expectedLength(nullptr, 0));
}

void test_little_endian_helpers_roundtrip(void) {
  uint8_t buf[4] = {0};

  FrameCodec::writeU32LE(0xDEADBEEFu, buf);
  TEST_ASSERT_EQUAL_HEX8(0xEF, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0xBE, buf[1]);
  TEST_ASSERT_EQUAL_HEX8(0xAD, buf[2]);
  TEST_ASSERT_EQUAL_HEX8(0xDE, buf[3]);
  TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFu, FrameCodec::readU32LE(buf));

  const float kPi = 3.14159f;
  FrameCodec::writeFloatLE(kPi, buf);
  float out = FrameCodec::readFloatLE(buf);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, kPi, out);
}

void test_zero_payload_roundtrip(void) {
  auto frame = makeFrame(0x0100, 0x7FFF, {});
  TEST_ASSERT_EQUAL_size_t(
      FrameCodec::kHeaderSize + FrameCodec::kDataChecksum, frame.size());

  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
  TEST_ASSERT_EQUAL_UINT16(0, out.data_len);
  TEST_ASSERT_EQUAL_UINT16(0x7FFF, out.type);
}

void test_assembler_recovers_valid_frame_after_boot_garbage(void) {
  // Issue #8: power-on UART noise contains 0x01 bytes that are not SOF.
  auto heart = makeFrame(0x0001, 0x0A15, {0x00, 0x00, 0xC8, 0x42});  // 100.0f
  std::vector<uint8_t> stream = {0x00, 0xFF, 0x01, 0x02, 0x03, 0x04,
                                 0x05, 0x06, 0x07, 0x08};
  stream.insert(stream.end(), heart.begin(), heart.end());

  FrameAssembler asmblr;
  std::vector<std::vector<uint8_t>> frames;
  asmblr.feedBuffer(stream.data(), stream.size(), frames);

  TEST_ASSERT_EQUAL_size_t(1, frames.size());
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok),
                    static_cast<int>(FrameCodec::tryDecode(
                        frames[0].data(), frames[0].size(), out)));
  TEST_ASSERT_EQUAL_UINT16(0x0A15, out.type);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 100.0f, FrameCodec::readFloatLE(out.data));
}

void test_assembler_rejects_oversized_len_without_eating_next_frame(void) {
  // Forged header: SOF + LEN=0x0200 (512) with a bad checksum, then a real frame.
  std::vector<uint8_t> forged = {0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                                 0x00};
  auto breath = makeFrame(0x0002, 0x0A14, {0x00, 0x00, 0x20, 0x41});  // 10.0f
  std::vector<uint8_t> stream = forged;
  stream.insert(stream.end(), breath.begin(), breath.end());

  FrameAssembler asmblr;
  std::vector<std::vector<uint8_t>> frames;
  asmblr.feedBuffer(stream.data(), stream.size(), frames);

  TEST_ASSERT_EQUAL_size_t(1, frames.size());
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok),
                    static_cast<int>(FrameCodec::tryDecode(
                        frames[0].data(), frames[0].size(), out)));
  TEST_ASSERT_EQUAL_UINT16(0x0A14, out.type);
}

void test_assembler_keeps_heart_rate_after_many_target_frames(void) {
  // Issue #7: people-count frames arrive more often than vitals. A delayed
  // host still needs the later heart-rate frame to assemble.
  auto target = makeFrame(0x0010, 0x0A04, {0x01, 0x00, 0x00, 0x00});
  auto heart  = makeFrame(0x00FF, 0x0A15, {0x00, 0x00, 0x90, 0x42});  // 72.0f

  std::vector<uint8_t> stream;
  for (int i = 0; i < 40; ++i) {
    stream.insert(stream.end(), target.begin(), target.end());
  }
  stream.insert(stream.end(), heart.begin(), heart.end());

  FrameAssembler asmblr;
  std::vector<std::vector<uint8_t>> frames;
  asmblr.feedBuffer(stream.data(), stream.size(), frames);

  TEST_ASSERT_EQUAL_size_t(41, frames.size());
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::Ok),
                    static_cast<int>(FrameCodec::tryDecode(
                        frames.back().data(), frames.back().size(), out)));
  TEST_ASSERT_EQUAL_UINT16(0x0A15, out.type);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 72.0f, FrameCodec::readFloatLE(out.data));
}

void test_assembler_sof_inside_payload_stays_in_same_frame(void) {
  const std::vector<uint8_t> payload = {0x01, 0x02, 0x01, 0x03};
  auto frame = makeFrame(0x0003, 0x0A16, payload);

  FrameAssembler asmblr;
  std::vector<std::vector<uint8_t>> frames;
  asmblr.feedBuffer(frame.data(), frame.size(), frames);

  TEST_ASSERT_EQUAL_size_t(1, frames.size());
  TEST_ASSERT_EQUAL_size_t(frame.size(), frames[0].size());
}

void test_try_decode_bounds_check_declared_length(void) {
  auto frame = makeFrame(0x0001, 0x0A14, {0x00, 0x00, 0x20, 0x41});
  // Claim a huge payload without supplying those bytes.
  frame[3] = 0x01;
  frame[4] = 0x00;
  DecodedFrame out{};
  TEST_ASSERT_EQUAL(static_cast<int>(Status::FrameTooShort),
                    static_cast<int>(FrameCodec::tryDecode(
                        frame.data(), frame.size(), out)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_checksum_is_xor_then_invert);
  RUN_TEST(test_encode_layout_is_stable);
  RUN_TEST(test_encode_decode_roundtrip);
  RUN_TEST(test_decode_rejects_bad_sof);
  RUN_TEST(test_decode_rejects_short_buffer);
  RUN_TEST(test_decode_rejects_header_checksum_mismatch);
  RUN_TEST(test_decode_rejects_data_checksum_mismatch);
  RUN_TEST(test_encode_rejects_oversized_payload);
  RUN_TEST(test_encode_rejects_tiny_output_buffer);
  RUN_TEST(test_expected_length_from_partial_header);
  RUN_TEST(test_little_endian_helpers_roundtrip);
  RUN_TEST(test_zero_payload_roundtrip);
  RUN_TEST(test_assembler_recovers_valid_frame_after_boot_garbage);
  RUN_TEST(test_assembler_rejects_oversized_len_without_eating_next_frame);
  RUN_TEST(test_assembler_keeps_heart_rate_after_many_target_frames);
  RUN_TEST(test_assembler_sof_inside_payload_stays_in_same_frame);
  RUN_TEST(test_try_decode_bounds_check_declared_length);
  return UNITY_END();
}
