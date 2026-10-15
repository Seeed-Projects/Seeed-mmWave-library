/**
 * @file Seeed_Arduino_mmWave.h
 * @date  02 July 2024

 * @author Spencer Yan
 *
 * @note Description of the file
 *
 * @copyright © 2024, Seeed Studio
 */

#ifndef Seeed_Arduino_mmWave_H
#define Seeed_Arduino_mmWave_H

#define _VERSION_MMWAVEBREATH_0_0_1 "1.0.1"

#include "SEEED_MR60BHA2.h"
#include "SEEED_MR60FDA2.h"

typedef enum {
  MMWAVE_DEVICE_RESERVE = 0,
#ifdef SEEED_MR60BHA2_H
  MMWAVE_FALL_MR60BHA2,
#endif
#ifdef SEEED_MR60FDA2_H
  MMWAVE_BREATH_MR60FDA2,
#endif
} MMWAVE_DEVICE;

#endif /*Seeed_Arduino_mmWave_H*/