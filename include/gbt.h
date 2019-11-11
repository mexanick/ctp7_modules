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
 *  \return Returns a 2-dimensional array containing each phase of each VFAT in each OptoHybrid. The precise error is logged and written to the `LOG4CPLUS` logger.
 *
 *  \detail
 *
 *  The scan seeks for valid RX phases for the VFAT's of one OptoHybrid. A phase is considered valid when `LINK_GOOD = 1`, `SYNC_ERR_CNT = 0` and `CFG_RUN != 0xdeaddead`. In order to improve the reliability of the scan, it is repeated `N` times.
 *
 *  The results are returned as a 2-dimensional array in ``std::vector<std::vector<uint32_t>>`` format, where there is one vector entry per OptoHybrid index, and each entry of the sub-vectors corresponds to the phase read at that VFAT index within the OptoHybrid.
 *  Each key is a word array of 16 elements for the 16 possible phases ordered from phase 0 to phase 15.
 *  Each word is the number of time the scan was "good" out of the total number of scan requested, N.
 *
 */
struct scanGBTPhases : public xhal::rpc::Method
{
    std::vector<std::vector<uint32_t>> operator()(const uint32_t &ohN, const uint32_t &nResets = 1, const uint8_t &phaseMin = gbt::PHASE_MIN, const uint8_t &phaseMax = gbt::PHASE_MAX, const uint8_t &phaseStep = 1, const uint32_t &nVerificationReads = 10) const;
};

/*! \brief Scan the GBT phases of one OptoHybrid.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] config Configuration blob of the GBT. This is a 366 elements long array whose each element is the value of one register sorted from address 0 to address 365.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `LOG4CPLUS` logger.
 */
struct writeGBTConfig : public xhal::rpc::Method
{
    bool operator()(const uint32_t &ohN, const uint32_t &gbtN) const;
};

/*! \brief Write the phase of a single VFAT.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] vfatN VFAT index of which the phase is changed.
 *  \param[in] phase Phase value to write. Minimal phase is 0 and maximal phase is 15.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `LOG4CPLUS` logger.
 */
struct writeGBTPhase : public xhal::rpc::Method
{
    bool operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint8_t &phase) const;
};

/*! \brief This function writes a single register in the given GBT of the given OptoHybrid.
 *  \param[in] ohN OptoHybrid index number. Warning : the value of this parameter is not checked because of the cost of a register access.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] address GBT register address to write. The highest writable address is 365.
 *  \param[in] value Value to write to the GBT register.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `LOG4CPLUS` logger.
 */
bool writeGBTRegLocal(const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value);
