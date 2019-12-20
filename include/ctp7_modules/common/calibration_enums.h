#ifndef COMMON_CALIBRATION_ENUMS_H
#define COMMON_CALIBRATION_ENUMS_H

#include <stdint.h>

namespace calibration {

  /*!
   * \brief VFAT calibration mode
   *
   * Modes in which the VFAT calibration circuit can be configured
   */
  enum struct VFATCalibrationMode : uint8_t {
    DISABLED = 0x0,  ///< Disable calibration mode
    VOLTAGE  = 0x1,  ///< Calibration circuit operates in voltage pulse mode
    CURRENT  = 0x2,  ///< Calibration circuit operates in current source mode
  };
}
#endif
