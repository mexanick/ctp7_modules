#ifndef SERVER_CALIBRATION_ROUTINES_H
#define SERVER_CALIBRATION_ROUTINES_H

#include <map>

namespace calibration {
  /*!
   *  \brief Unmask the channel of interest and masks all the other
   *
   *  \param \c ohN OptoHybrid optical link number
   *  \param \c vfatN VFAT position
   *  \param \c ch Channel of interest
   *
   *  \returns Original channel masks in the form of an \c std::unordered_map <chanMaskAddr, mask>
   */
  std::unordered_map<uint32_t, uint32_t> setSingleChanMask(const uint8_t &ohN, const uint8_t &vfatN, const uint8_t &ch);

  /*!
   *  \brief Applies channel masks to
   *
   *  \param \c channelMasks unordered map of channel masks as obtained from \c setSingleChanMask
   */
  void applyChanMask(const std::unordered_map<uint32_t, uint32_t> &channelMasks);

}

#endif
