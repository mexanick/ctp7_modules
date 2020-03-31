#ifndef SERVER_VFAT3_H
#define SERVER_VFAT3_H

namespace vfat3 {

  /*!
   *  \brief Decode a Reed--Muller encoded VFAT3 ChipID
   *
   *  \param \c encChipID 32-bit encoded chip ID to decode
   *
   *  \returns decoded VFAT3 chip ID
   */
  uint16_t decodeChipID(const uint32_t &encChipID);

}

#endif
