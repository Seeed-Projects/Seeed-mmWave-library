/**
 * @file SEEED_MR60FDA2.h
 * @date  02 July 2024

 * @author Spencer Yan
 *
 * @note Seeed Studio MR60FDA2
 *
 * @copyright © 2024, Seeed Studio
 */

#ifndef SEEED_MR60FDA2_H
#define SEEED_MR60FDA2_H

#include "SeeedmmWave.h"

enum class TypeFallDetection : uint16_t {
  UserLogInfo = 0x010E,

  ReportFallDetection = 0x0E02,  // is_fall
  InstallationHeight  = 0x0E04,
  RadarParameters     = 0x0E06,
  FallThreshold       = 0x0E08,
  FallSensitivity     = 0x0E0A,

  HeightUpload                  = 0x0E0E,
  AlarmParameters               = 0x0E0C,
  RadarInitSetting              = 0x2110,
  Report3DPointCloudDetection   = 0x0A08,
  Report3DPointCloudTargetInfo = 0x0A04,
  ReportUnmannedDetection       = 0x0F09,
};

class SEEED_MR60FDA2 : public SeeedmmWave {
 private:
  /*  get parameters */
  bool _parametersValid = false; /* flag */
  float _height;
  float _thershold;  // deault value=0.6m
  uint32_t _sensitivity;
  float _rect_XL;
  float _rect_XR;
  float _rect_ZF;
  float _rect_ZB;

  /*set height*/
  bool _isHeightValid = false;
  bool _isThresholdValid   = false;
  bool _isAlarmAreaValid   = false;
  bool _isSensitivityValid = false;
  bool _isHuman            = false;

  /* get fall detection*/
  bool _isFall = false;

  /* additional flags*/
  bool _isHumanValid = false;
  bool _isFallValid  = false;

  bool getFallInternal();
 protected:
  bool getRadarParameters();

 public:
  SEEED_MR60FDA2() {}

  virtual ~SEEED_MR60FDA2() {}

  bool handleType(uint16_t _type, const uint8_t* data,
                  size_t data_len) override;

  bool resetSetting(void);

  bool setUserLog(bool flag);

  bool setInstallationHeight(const float height);
  bool setThreshold(const float threshold);
  bool setSensitivity(const uint32_t _sensitivity);
  bool setAlamArea(const float rect_XL, const float rect_XR,
                   const float rect_ZF, const float rect_ZB);

  bool getRadarParameters(float& height, float& threshold,
                          uint32_t& sensitivity);
  bool getRadarParameters(float& height, float& threshold,
                          uint32_t& sensitivity, float& rect_XL, float& rect_XR,
                          float& rect_ZF, float& rect_ZB);

  // bool get3DPointCloud(const int option);

  bool getFall(bool &is_fall);
  bool getHuman(bool &is_human);
  bool getFall();
  bool getHuman();
};

#endif /*SEEED_MR60FDA2_H*/