/*! \file
 *  \brief RPC module for GBT methods
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef GBT_H
#define GBT_H

#include "hw_constants.h"

#include "utils.h"

/*! \brief Scan the GBT phases of one OptoHybrid.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] nResets The number of times the link reset and subsequent repeatedRegRead will be performed
 *  \param[in] phaseMin Lowest phase to scan (min = 0).
 *  \param[in] phaseMax Highest phase to scan (max = 15).
 *  \param[in] phaseStep Step to scan the phases.
 *  \param[in] nVerificationReads The number of reads for repeatedRegRead method
 *  \return Returns a 2-dimensional array containing each phase of each VFAT in the OptoHybrid specified by ``ohN``.
 *
 *  \detail
 *
 *  The scan seeks for valid RX phases for the VFAT's of one OptoHybrid. A phase is considered valid when `LINK_GOOD = 1`, `SYNC_ERR_CNT = 0` and `CFG_RUN != 0xdeaddead`. In order to improve the reliability of the scan, it is repeated `nScans` times.
 *
 *  The results are returned as a 2-dimensional array of GBT phases in ``std::vector<std::vector<uint32_t>>`` format, where the first index refers to the OptoHybrid index, and the second index corresponds to the VFAT index within the OptoHybrid.
 *  Each key is a word array of 16 elements for the 16 possible phases ordered from phase 0 to phase 15.
 *  Each word is the number of time the scan was "good" out of the total number of scan requested, nScans.
 *
 *  An error message of 
 *  ```
 *  std::range_error: "The ohN parameter supplied (<ohN>) exceeds the number of OH's supported by the CTP7 (<ohMax>)."
 *  ```
 *  means that the CTP7 doesn't support that many OptoHybrids. Try again with a lower OptoHybrid number or on a different CTP7. ``ohMax`` is read from the register: ``"GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH"``.
 *
 */
struct scanGBTPhases : public xhal::common::rpc::Method
{
    std::vector<std::vector<uint32_t>> operator()(const uint32_t &ohN, const uint32_t &nResets = 1, const uint8_t &phaseMin = gbt::PHASE_MIN, const uint8_t &phaseMax = gbt::PHASE_MAX, const uint8_t &phaseStep = 1, const uint32_t &nVerificationReads = 10) const;
};

/*! \brief Write the GBT configuration of one OptoHybrid.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] config Configuration blob of the GBT. This is a 366 elements long array whose each element is the value of one register sorted from address 0 to address 365.
 *  \detail 
 *
 *  An error message of 
 *  ```
 *  std::range_error: "The ohN parameter supplied (<ohN>) exceeds the number of OH's supported by the CTP7 (<ohMax>)."
 *  ```
 *  means that the CTP7 doesn't support that many OptoHybrids. Try again with a lower OptoHybrid number or on a different CTP7. ``ohMax`` is read from the register: ``"GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH"``.
 *  
 *  An error message of 
 *  ```
 *  std::range_error: "The gbtN parameter supplied (<gbtN>) exceeds the number of GBT's per OH (<GBTS_PER_OH>)."
 *  ```
 *  means that the OptoHybrid doesn't support that number of GBT's. Try again with a lower GBT number or different OptoHybrid. gbt::GBTS_PER_OH is defined in hw_constants.h
 *
 */
struct writeGBTConfig : public xhal::common::rpc::Method
{
    void operator()(const uint32_t &ohN, const uint32_t &gbtN, const config_t &config) const;
};

/*! \brief Write the phase of a single VFAT.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] vfatN VFAT index of which the phase is changed.
 *  \param[in] phase Phase value to write. Minimal phase is 0 and maximal phase is 15.
 *  \detail 
 *
 *  An error message of 
 *  ```
 *  std::range_error: "The ohN parameter supplied (<ohN>) exceeds the number of OH's supported by the CTP7 (<ohMax>)."
 *  ```
 *  means that the CTP7 doesn't support that many OptoHybrids. Try again with a lower OptoHybrid number or on a different CTP7. ``ohMax`` is read from the register: ``"GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH"``.
 *  
 *  An error message of 
 *  ```
 *  std::range_error: "The vfatN parameter supplied (<vfatN>) exceeds the number of VFAT's per OH (<VFATS_PER_OH>)."
 *  ```
 *  means that the OptoHybrid doesn't support that number of VFAT's. Try again with a lower VFAT number or different OptoHybrid. oh::VFATS_PER_OH is defined in hw_constants.h
 *
 */
struct writeGBTPhase : public xhal::common::rpc::Method
{
    void operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint8_t &phase) const;
};

/*! \brief This function writes a single register in the given GBT of the given OptoHybrid.
 *  \param[in] ohN OptoHybrid index number. Warning : the value of this parameter is not checked because of the cost of a register access.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] address GBT register address to write. The highest writable address is 365.
 *  \param[in] value Value to write to the GBT register.
 *  \detail
 *
 *  An error message of 
 *  ```
 *  std::range_error: "The gbtN parameter supplied (<gbtN>) exceeds the number of GBT's per OH (<GBTS_PER_OH>)."
 *  ```
 *  means that the OptoHybrid doesn't support that number of GBT's. Try again with a lower GBT number or different OptoHybrid. gbt::GBTS_PER_OH is defined in hw_constants.h
 *
 *  An error message of 
 *  ```
 *  std::range_error: "GBT has <N_ADDRESSES> writable addresses while the provided address is <address>."
 *  ```
 *  means that the GBT doesn't have enough writable addresses. Try again with a lower address. ``N_ADDRESSES`` is one less than gbt::CONFIG_SIZE in hw_constants.h
 *
 */
void writeGBTReg(const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value);
