/**
 * @file SEEED_MR60BHA2.h
 * @date  02 July 2024
 * @author Spencer Yan
 *
 * @note Description of the file
 *
 * @copyright © 2024, Seeed Studio
 *
 * @attention MR60BHA2 module is used to monitor breath and heart rate
 */

#ifndef SEEED_MR60BHA2_H
#define SEEED_MR60BHA2_H

#include "SeeedmmWave.h"

#define MAX_TARGET_NUM    3
#define MAX_POINT_CLOUD_NUM 32

#define RANGE_STEP 17.28f

enum class TypeHeartBreath : uint16_t {
  TypeHeartBreathPhase    = 0x0A13,
  TypeBreathRate          = 0x0A14,
  TypeHeartRate           = 0x0A15,
  TypeHeartBreathDistance = 0x0A16,
  Report3DPointCloudDetection   = 0x0A08,
  Report3DPointCloudTargetInfo = 0x0A04,
  ReportHumanDetection       = 0x0F09,
  ReportFirmware          = 0xFFFF,
};

typedef struct HeartBreath {
  float total_phase;
  float breath_phase;
  float heart_phase;
} HeartBreath;

typedef struct TargetN {
  float x_point;
  float y_point;
  int32_t dop_index;
  int32_t cluster_index;
} TargetN;

typedef struct FirmwareVersion {
  uint8_t project_name;
  uint8_t major_version;
  uint8_t sub_version;
  uint8_t modified_version;
} FirmwareVersion;

typedef union FirmwareInfo {
  FirmwareVersion firmware_verson;
  uint32_t value;
};

typedef struct PeopleCounting {
  std::vector<TargetN> targets;
} PeopleCounting;

class SEEED_MR60BHA2 : public SeeedmmWave {
 private:
  /* HeartBreath */
  HeartBreath _heart_breath = {0};

  /* BreathRate */
  float _breath_rate = 0;

  /* HeartRate */
  float _heart_rate = 0;

  /* HeartBreathDistance */
  uint32_t _rangeFlag = 0;
  float _range        = 0;

  /* HumanDetection */
  bool _isHumanDetected      = false;  // last reported presence
  bool _isHumanDetectionValid = false;

  /* PeopleCounting PointCloud */
  PeopleCounting _people_counting_point_cloud;
  bool _isPeopleCountingPointCloudValid = false;

  /* PeopleCounting TargetInfo */
  PeopleCounting _people_counting_target_info;
  bool _isPeopleCountingTargetInfoValid = false;


  FirmwareInfo _firmware_info;
  bool _isFirmwareInfoValid     = false;

  bool _isHeartBreathPhaseValid = false;
  bool _isBreathRateValid       = false;
  bool _isHeartRateValid        = false;
  bool _isDistanceValid         = false;

 public:
  SEEED_MR60BHA2() {}

  virtual ~SEEED_MR60BHA2() {}

  bool handleType(uint16_t _type, const uint8_t* data,
                  size_t data_len) override;

  bool getHeartBreathPhases(float& total_phase, float& breath_phase,
                            float& heart_phase);
  bool getBreathRate(float& rate);
  bool getHeartRate(float& rate);
  bool getDistance(float& distance);
  bool getPeopleCountingPointCloud(PeopleCounting& point_cloud);
  bool getPeopleCountingTargetInfo(PeopleCounting& target_info);
  bool isHumanDetected();
  bool getFirmwareInfo(FirmwareInfo& firmware_info);

 private:
  bool extractTargets(const uint8_t* data, size_t data_len, size_t max_keep,
                      std::vector<TargetN>& out);
};

#endif /*SEEED_MR60BHA2_H*/