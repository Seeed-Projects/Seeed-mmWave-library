#include "SEEED_MR60BHA2.h"

namespace {
constexpr size_t kTargetNSize =
    sizeof(float) + sizeof(float) + sizeof(int32_t) + sizeof(int32_t);
}  // namespace

/**
 * @brief Copy at most `max_keep` TargetN records from a people-counting payload.
 * @param data Payload bytes (count + records).
 * @param data_len Payload length.
 * @param max_keep Allocation cap (people vs point-cloud).
 * @param out Destination vector.
 * @return true when the count field is present.
 *
 * 从人数/点云载荷里按长度上限拷贝目标记录。
 */
bool SEEED_MR60BHA2::extractTargets(const uint8_t* data, size_t data_len,
                                    size_t max_keep,
                                    std::vector<TargetN>& out) {
  if (data_len < sizeof(uint32_t)) {
    return false;
  }
  size_t target_num          = extractU32(data);
  const size_t available = (data_len - sizeof(uint32_t)) / kTargetNSize;
  if (target_num > available) {
    target_num = available;
  }
  if (target_num > max_keep) {
    target_num = max_keep;
  }

  data += sizeof(uint32_t);
  out.clear();
  out.reserve(target_num);
  for (size_t i = 0; i < target_num; ++i) {
    TargetN target;
    target.x_point       = extractFloat(data);
    data += sizeof(float);
    target.y_point       = extractFloat(data);
    data += sizeof(float);
    target.dop_index     = static_cast<int32_t>(extractU32(data));
    data += sizeof(int32_t);
    target.cluster_index = static_cast<int32_t>(extractU32(data));
    data += sizeof(int32_t);
    out.push_back(target);
  }
  return true;
}

/**
 * @brief Handle different types of heart and breath data.
 *
 * This function processes different types of heart and breath data based on
 * the type identifier.
 *
 * @param _type The type identifier of the data.
 * @param data The pointer to the data buffer.
 * @param data_len The length of the data buffer.
 * @return true if the data is handled successfully.
 * @return false if there is an error in handling the data.
 */
bool SEEED_MR60BHA2::handleType(uint16_t _type, const uint8_t* data,
                                size_t data_len) {
  TypeHeartBreath type = static_cast<TypeHeartBreath>(_type);
  switch (type) {
    case TypeHeartBreath::TypeHeartBreathPhase: {
      if (data_len < 3 * sizeof(float)) return false;
      _heart_breath.total_phase  = extractFloat(data);
      _heart_breath.breath_phase = extractFloat(data + sizeof(float));
      _heart_breath.heart_phase  = extractFloat(data + 2 * sizeof(float));
      _isHeartBreathPhaseValid   = true;
      break;
    }
    case TypeHeartBreath::TypeBreathRate: {
      if (data_len < sizeof(float)) return false;
      _breath_rate       = extractFloat(data);
      _isBreathRateValid = true;
      break;
    }
    case TypeHeartBreath::TypeHeartRate: {
      if (data_len < sizeof(float)) return false;
      _heart_rate       = extractFloat(data);
      _isHeartRateValid = true;
      break;
    }
    case TypeHeartBreath::TypeHeartBreathDistance: {
      if (data_len < sizeof(uint32_t) + sizeof(float)) return false;
      _rangeFlag       = extractU32(data);
      _range           = extractFloat(data + sizeof(uint32_t));
      _isDistanceValid = true;
      break;
    }
    case TypeHeartBreath::ReportHumanDetection: {
      if (data_len < 1) return false;
      _isHumanDetected       = data[0] != 0;
      _isHumanDetectionValid = true;
      break;
    }
    case TypeHeartBreath::Report3DPointCloudDetection: {
      if (!extractTargets(data, data_len, MAX_POINT_CLOUD_NUM,
                          _people_counting_point_cloud.targets)) {
        return false;
      }
      _isPeopleCountingPointCloudValid = true;
      break;
    }
    case TypeHeartBreath::Report3DPointCloudTargetInfo: {
      if (!extractTargets(data, data_len, MAX_TARGET_NUM,
                          _people_counting_target_info.targets)) {
        return false;
      }
      _isPeopleCountingTargetInfoValid = true;
      break;
    }
    case TypeHeartBreath::ReportFirmware: {
      if (data_len < sizeof(uint32_t)) return false;
      _firmware_info.value = extractU32(data);
      _isFirmwareInfoValid = true;
      break;
    }
    default:
      return false;  // Unhandled type
  }
  return true;
}

bool SEEED_MR60BHA2::getHeartBreathPhases(float& total_phase,
                                          float& breath_phase,
                                          float& heart_phase) {
  if (!_isHeartBreathPhaseValid)
    return false;
  _isHeartBreathPhaseValid = false;

  total_phase  = _heart_breath.total_phase;
  breath_phase = _heart_breath.breath_phase;
  heart_phase  = _heart_breath.heart_phase;
  return true;
}

bool SEEED_MR60BHA2::getBreathRate(float& rate) {
  if (!_isBreathRateValid)
    return false;
  _isBreathRateValid = false;
  rate               = _breath_rate;
  return true;
}

bool SEEED_MR60BHA2::getHeartRate(float& rate) {
  if (!_isHeartRateValid)
    return false;
  _isHeartRateValid = false;
  rate              = _heart_rate;
  return true;
}

bool SEEED_MR60BHA2::getDistance(float& distance) {
  if (!_isDistanceValid)
    return false;
  _isDistanceValid = false;
  if (!_rangeFlag)
    return false;
  distance = _range;
  return true;
}

bool SEEED_MR60BHA2::getPeopleCountingPointCloud(PeopleCounting& point_cloud) {
  if (!_isPeopleCountingPointCloudValid)
    return false;
  _isPeopleCountingPointCloudValid = false;
  point_cloud = std::move(_people_counting_point_cloud);
  return true;
}

bool SEEED_MR60BHA2::getPeopleCountingTargetInfo(PeopleCounting& target_info) {
  if (!_isPeopleCountingTargetInfoValid)
    return false;
  _isPeopleCountingTargetInfoValid = false;
  target_info = std::move(_people_counting_target_info);
  return true;
}

bool SEEED_MR60BHA2::isHumanDetected() {
  return _isHumanDetected;
}

bool SEEED_MR60BHA2::getFirmwareInfo(FirmwareInfo& firmware_info) {
  if (!_isFirmwareInfoValid)
    return false;
  _isFirmwareInfoValid = false;
  firmware_info = std::move(_firmware_info);
  return true;  
}
