#ifndef SEEEDMMWAVE_H
#define SEEEDMMWAVE_H

#include <Arduino.h>
#include <stdint.h>

#ifdef ESP_ARDUINO_VERSION_MAJOR
#  if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
// Code for version 3.x
#  else
// Code for version 2.x
#  endif
#else
// Code for version 1.x
#endif

#ifndef ESP32
#  error "Currently this library only supports ESP32"
#endif

#include <memory>
#include <queue>

#include "FrameAssembler.h"

#ifndef _MMWAVE_DEBUG
  #define _MMWAVE_DEBUG 0
#endif

#ifndef _UART_BAUD
#  define _UART_BAUD 115200
#else
#  warning "Notice: the uart baud of mmWave serial should be 115200"
#endif

#define MAX_QUEUE_SIZE    10
#define FRAME_BUFFER_SIZE 512
#define SOF_BYTE          0x01

// Frame structure sizes
#define SIZE_SOF        1
#define SIZE_ID         2
#define SIZE_LEN        2
#define SIZE_TYPE       2
#define SIZE_HEAD_CKSUM 1
#define SIZE_FRAME_HEADER                                                      \
  (SIZE_SOF + SIZE_ID + SIZE_LEN + SIZE_TYPE + SIZE_HEAD_CKSUM)
#define SIZE_DATA_CKSUM 1

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define SEEED_WAVE_IS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define SEEED_WAVE_IS_BIG_ENDIAN 0
#else
#  warning "Unable to determine the size end system"
#endif

#ifndef ESP32

#endif

#define MMWaveMaxQueueSize 120

class SeeedmmWave {
 private:
  HardwareSerial* _serial = nullptr;
  uint32_t _baud          = 0;
  uint32_t _wait_delay    = 0;

  std::queue<std::vector<uint8_t>> byteQueue;
  seeed::mmwave::FrameAssembler _assembler;

  void enqueueFrame(std::vector<uint8_t>&& frame);
  void drainSerial();

 protected:
  size_t expectedFrameLength(const std::vector<uint8_t>& buffer);
  uint8_t calculateChecksum(const uint8_t* data, size_t len);
  bool validateChecksum(const uint8_t* data, size_t len,
                        uint8_t expected_checksum);
  float extractFloat(const uint8_t* bytes) const;
  uint32_t extractU32(const uint8_t* bytes) const;
  void floatToBytes(float value, uint8_t* bytes);
  void uint32ToBytes(uint32_t value, uint8_t* bytes);

  bool processFrame(const uint8_t* frame_bytes, size_t len,
                    uint16_t data_type = 0xFFFF);
  /**
   * @brief Handle different types of data frames.
   *
   * This is a pure virtual function that must be overridden by derived classes
   * to handle specific types of data frames. The function processes the data
   * based on the type and data length, and performs necessary actions.
   *
   * @param _type The type of the data frame.
   * @param data The pointer to the data buffer.
   * @param data_len The length of the data buffer.
   * @return true if the data is processed successfully, false otherwise.
   */
  virtual bool handleType(uint16_t _type, const uint8_t* data,
                          size_t data_len) = 0;

  std::vector<uint8_t> packetFrame(uint16_t type, const uint8_t* data = nullptr,
                                   size_t len = 0);
  bool sendFrame(const std::vector<uint8_t>& frame);

 public:
  SeeedmmWave() {}
  virtual ~SeeedmmWave() {
    if (_serial) {
      _serial->end();
      _serial = nullptr;
    }
  }

  void begin(HardwareSerial* serial, uint32_t baud = _UART_BAUD,
             uint32_t wait_delay = 1, int rst = -1);

  /**
   * @brief Drop in-flight UART bytes and restart frame lock.
   *
   * 清空串口残留并重新锁定帧头，用于上电噪声或失步后的恢复。
   */
  void resync();

  int available();
  int read(void);
  int read(char* data, int length);
  size_t write(const uint8_t* buffer, size_t size);
  size_t write(const char* buffer, size_t size);

  /**
   * @brief Send a frame of data.
   *
   * @attention This function constructs and sends a frame of data, including
   * the frame header, data, and checksums.
   *
   * @param type The type of the frame.
   * @param data The data to include in the frame. Defaults to nullptr.
   * @param len The length of the data. Defaults to 0.
   * @retval True if the frame is sent successfully
   * @retval False otherwise.
   */
  bool update(uint32_t timeout = 100);
  void fetch(uint32_t timeout = 1000);
  bool fetchType(uint16_t data_type = 0xFFFF, uint32_t timeout = 1000);
  bool send(uint16_t type, const uint8_t* data = nullptr, size_t data_len = 0);

  bool processQueuedFrames(uint16_t data_type = 0xFFFF,
                           uint32_t timeout   = 1000);

};

void printHexBuff(const std::vector<uint8_t>& buffer);

#endif  // SEEEDMMWAVE_H
