#ifndef SERVER_GBT_H
#define SERVER_GBT_H

namespace gbt {

  /*!
   *  \brief Writes a single register in the given GBT of the given OptoHybrid.
   *
   *  \detail FIXME
   *
   *  An error message of
   *  ```
   *  std::range_error: "The gbtN parameter supplied (<gbtN>) exceeds the number of GBT's per OH (<GBTS_PER_OH>)."
   *  ```
   *  means that the OptoHybrid doesn't support that number of GBT's.
   *  Try again with a lower GBT number or different OptoHybrid.
   *  gbt::GBTS_PER_OH is defined in hw_constants.h
   *
   *  An error message of
   *  ```
   *  std::range_error: "GBT has <N_ADDRESSES> writable addresses while the provided address is <address>."
   *  ```
   *  means that the GBT doesn't have enough writable addresses.
   *  Try again with a lower address. ``N_ADDRESSES`` is one less than gbt::CONFIG_SIZE in hw_constants.h
   *
   *  \throws \c std::range_error if
   *            * the specified \c gbtN is larger than that supported by the FW
   *            * the specified \c address is larger than largest address in the GBT configuration space
   *
   *  \param[in] ohN OptoHybrid index number. Warning : the value of this parameter is not checked because of the cost of a register access.
   *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
   *  \param[in] address GBT register address to write. The highest writable address is 365.
   *  \param[in] value Value to write to the GBT register.
   */
  void writeGBTReg(uint32_t ohN, uint32_t gbtN, uint16_t address, uint8_t value);

}

#endif
