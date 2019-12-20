/*!
 *  \brief AMC BLASTER RAM local only methods
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef SERVER_AMC_BLASTER_RAM_H
#define SERVER_AMC_BLASTER_RAM_H

#include "ctp7_modules/common/amc/blaster_ram_defs.h"

namespace amc {
  namespace blaster {
    /*!
     *  \brief Verify the size of the provided BLOB size for a specified RAM in the BLASTER module
     *
     *  \param \c type Select which RAM to compare the BLOB size to, allowed values are:
     *           * \c BLASTERType::GBT
     *           * \c BLASTERType::OptoHybrid
     *           * \c BLASTERType::VFAT
     *           * \c BLASTERType::ALL
     *  \param \c sz Size of the BLOB that will be written
     *
     *  \returns \c true if the size is correct for the specified RAM
     */
    bool checkBLOBSize(BLASTERType const& type, size_t const& sz);

    /*!
     *  \brief Extract the starting address of the RAM for a specified component
     *
     *  \throws \c std::runtime_error if \c ohN > oh::VFATS_PER_OH-1
     *          or if the specified \partN is out of range
     *            * \c VFAT > oh::VFATS_PER_OH-1
     *            * \c GBT  > gbt::GBTS_PER_OH-1
     *
     *  \param \c type Select which RAM to obtain the addresss of, allowed values are:
     *           * \c BLASTERType::GBT
     *           * \c BLASTERType::OptoHybrid
     *           * \c BLASTERType::VFAT
     *           * \c BLASTERType::ALL
     *  \param \c ohN Select which OptoHybrid the component is associated with
     *  \param \c partN Select which VFAT/GBTx the configuration is for, default is 0, ignored for type \c BLASTERType::OptoHybrid
     *
     *  \returns address in the block RAM specified
     */
    uint32_t getRAMBaseAddr(BLASTERType const& type, uint8_t const& ohN, uint8_t const& partN=0);

  }
}

#endif
