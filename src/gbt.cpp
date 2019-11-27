/*! \file
 *  \brief RPC module for GBT methods
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#include "gbt.h"

#include "hw_constants.h"
#include "hw_constants_checks.h"
#include "moduleapi.h"
#include "memhub.h"
#include "utils.h"

#include <array>
#include <thread>
#include <chrono>

std::vector<std::vector<uint32_t>> scanGBTPhases::operator()(const uint32_t &ohN, const uint32_t &nResets, const uint8_t &phaseMin, const uint8_t &phaseMax, const uint8_t &phaseStep, const uint32_t &nVerificationReads) const
{
    LOG4CPLUS_INFO(logger, "Scanning the phases for OH #" << ohN << ".");

    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        std::stringstream errmsg;
        errmsg << "The ohN parameter supplied (" << ohN << ") exceeds the number of OH's supported by the CTP7 (" << ohMax << ").";
        throw std::range_error(errmsg.str());

    // phaseMin and phaseMax check
    checkPhase(phaseMin);
    checkPhase(phaseMax);

    // Results array
    std::vector<std::vector<uint32_t>> results(oh::VFATS_PER_OH, std::vector<uint32_t>(16));

    // Perform the scan
    for (uint8_t phase = phaseMin; phase <= phaseMax; phase += phaseStep) {
        // Set the new phases
        for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
            writeGBTPhase{}(ohN, vfatN, phase);
        }

        // Wait for the phases to be set
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::stringstream syncErrCnt_regName, cfgRun_regName, hwIdVer_regName, hwId_regName;
        for (uint32_t repN = 0; repN < nResets; repN++) {
            // Try to synchronize the VFAT's
            writeReg(la, "GEM_AMC.GEM_SYSTEM.CTRL.LINK_RESET", 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Check the VFAT status
            slowCtrlErrCntVFAT vfatErrs;
            for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
                syncErrCnt_regName << "GEM_AMC.OH_LINKS.OH" << ohN << ".VFAT" << vfatN << ".SYNC_ERR_CNT";
                cfgRun_regName << "GEM_AMC.OH.OH" << ohN << ".GEB.VFAT" << vfatN << ".CFG_RUN";
                hwIdVer_regName << "GEM_AMC.OH.OH" << ohN << ".GEB.VFAT" << vfatN << ".HW_ID_VER";
                hwId_regName << "GEM_AMC.OH.OH" << ohN << ".GEB.VFAT" << vfatN << ".HW_ID";
                vfatErrs = repeatedRegRead(syncErrCnt_regName, true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }
                vfatErrs = repeatedRegRead(cfgRun_regName, true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }
                vfatErrs = repeatedRegRead(hwIdVer_regName, true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }
                vfatErrs = repeatedRegRead(hwId_regName, true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }
                // If no errors, the phase is good
                results[vfatN][phase]++;
            }
        }
    }

    return results;

}

void writeGBTConfig::operator()(const uint32_t &ohN, const uint32_t &gbtN, const config_t &config) const
{
    std::stringstream logOH_GBT_config;
    logOH_GBT_config << "Writing the configuration of OH #" << ohN << " - GBTX #" << gbtN << ".";
    LOG4CPLUS_INFO(logger, logOH_GBT_config);

    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    std::stringstream errmsg;
    if (ohN >= ohMax)
        errmsg << "The ohN parameter supplied (" << ohN << ") exceeds the number of OH's supported by the CTP7 (" << ohMax << ").";
        throw std::range_error(errmsg.str());

    // gbtN check
    if (gbtN >= GBTS_PER_OH)
        errmsg << "The gbtN parameter supplied (" << ohN << ") exceeds the number of GBT's per OH (" << GBTS_PER_OH << ").";
        throw std::range_error(errmsg.str());

    // Write all the registers
    for (size_t address = 0; address < CONFIG_SIZE; address++) {
        writeGBTReg(ohN, gbtN, static_cast<uint16_t>(address), config[address]);
    }
}

void writeGBTPhase::operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint8_t &phase) const
{
    std::stringstream logOH_vfatphase;
    logOH_vfatphase << "Writing phase " << phase << " to VFAT #" << vfatN << " of OH #" << ohN << ".";
    LOG4CPLUS_INFO(logger, logOH_vfatphase);
    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    std::stringstream errmsg;
    if (ohN >= ohMax)
        errmsg << "The ohN parameter supplied (" << ohN << ") exceeds the number of OH's supported by the CTP7 (" << ohMax << ").";
        throw std::range_error(errmsg.str());

    // vfatN check
    if (vfatN >= oh::VFATS_PER_OH)
        errmsg << "The vfatN parameter supplied (" << vfatN << ") exceeds the number of VFAT's per OH (" << oh::VFATS_PER_OH << ").";
        throw std::range_error(errmsg.str());

    // phase check
    checkPhase(phase);

    // Write the triplicated phase registers
    const uint32_t gbtN = gbt::elinkMappings::VFAT_TO_GBT[vfatN];
    LOG4CPLUS_INFO(logger, stdsprintf("Writing %u to the VFAT #%u phase of GBT #%u, on OH #%u.", phase, vfatN, gbtN, ohN));

    for (unsigned char regN = 0; regN < 3; regN++) {
        const uint16_t regAddress = elinkMappings::ELINK_TO_REGISTERS[elinkMappings::VFAT_TO_ELINK[vfatN]][regN];

        writeGBTReg(ohN, gbtN, regAddress, phase);
    }

}

void writeGBTReg(const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value)
{
    // Check that the GBT exists
    std::stringstream errmsg;
    if (gbtN >= GBTS_PER_OH)
        errmsg << "The gbtN parameter supplied (" << gbtN << ") is larger than the number of GBT's per OH (" << GBTS_PER_OH << ").";
        throw std::range_error(errmsg.str());

    // Check that the address is writable
    if (address >= CONFIG_SIZE)
        errmsg << "GBT has " << CONFIG_SIZE-1 << "writable addresses while the provided address is" << address << ".";
        throw std::range_error(errmsg.str());

    // GBT registers are 8 bits long
    writeReg("GEM_AMC.SLOW_CONTROL.IC.READ_WRITE_LENGTH", 1);

    // Select the link number
    const uint32_t linkN = ohN*GBTS_PER_OH + gbtN;
    writeReg("GEM_AMC.SLOW_CONTROL.IC.GBTX_LINK_SELECT", linkN);

    // Write to the register
    writeReg("GEM_AMC.SLOW_CONTROL.IC.ADDRESS", address);
    writeReg("GEM_AMC.SLOW_CONTROL.IC.WRITE_DATA", value);
    writeReg("GEM_AMC.SLOW_CONTROL.IC.EXECUTE_WRITE", 1);

}
extern "C" {
    const char *module_version_key = "gbt v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        initLogging();

        if (memhub_open(&memsvc) != 0) {
            auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
            LOG4CPLUS_ERROR(logger, LOG4CPLUS_TEXT("Unable to connect to memory service: ") << memsvc_get_last_error(memsvc));
            LOG4CPLUS_ERROR(logger, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }

        modmgr->register_method("gbt", "writeGBTConfig", writeGBTConfig);
        modmgr->register_method("gbt", "writeGBTPhase", writeGBTPhase);
        modmgr->register_method("gbt", "scanGBTPhases", scanGBTPhases);
    }
}
