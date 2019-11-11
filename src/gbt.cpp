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
    LOG4CPLUS_INFO(logger, stdsprintf("Scanning the phases for OH #%u.", ohN));

    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // phaseMin check
    if (checkPhase(phaseMin))
        return true;

    // phaseMax check
    if (checkPhase(phaseMax))
        return true;

    // Results array
    std::vector<std::vector<uint32_t>> results(oh::VFATS_PER_OH, std::vector<uint32_t>(16));

    // Perform the scan
    for (uint8_t phase = phaseMin; phase <= phaseMax; phase += phaseStep) {
        // Set the new phases
        for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
            if (writeGBTPhase{}(ohN, vfatN, phase))
                return true;
        }

        // Wait for the phases to be set
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        for (uint32_t repN = 0; repN < nResets; repN++) {
            // Try to synchronize the VFAT's
            writeReg(la, "GEM_AMC.GEM_SYSTEM.CTRL.LINK_RESET", 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Check the VFAT status
            slowCtrlErrCntVFAT vfatErrs;
            for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
                if (readReg(stdsprintf("GEM_AMC.OH_LINKS.OH%hu.VFAT%hu.SYNC_ERR_CNT", ohN, vfatN)) != 0) {
                    continue;
                }
                vfatErrs = repeatedRegRead(stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.CFG_RUN", ohN, vfatN), true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }

                // check HW_ID_VER
                vfatErrs = repeatedRegRead(stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.HW_ID_VER", ohN, vfatN), true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }

                // check HW_ID
                vfatErrs = repeatedRegRead(stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.HW_ID", ohN, vfatN), true, nVerificationReads);
                if (vfatErrs.sum != 0) {
                    continue;
                }
                results[vfatN][phase]++;
            }
        }
    }

    return results;

} //End scanGBTPhase

bool writeGBTConfig::operator()(const uint32_t &ohN, const uint32_t &gbtN) const
{

    LOG4CPLUS_INFO(logger, stdsprintf("Writing the configuration of OH #%u - GBTX #%u.", ohN, gbtN));

    config_t config{};
    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // gbtN check
    if (gbtN >= GBTS_PER_OH)
        EMIT_RPC_ERROR(stdsprintf("The gbtN parameter supplied (%u) exceeds the number of GBT's per OH (%u).", gbtN, GBTS_PER_OH), true);

    // Write all the registers
    for (size_t address = 0; address < CONFIG_SIZE; address++) {
        if (writeGBTRegLocal(ohN, gbtN, static_cast<uint16_t>(address), config[address]))
            return true;
    }

    return false;

} //End writeGBTConfig

bool WriteGBTPhase::operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint8_t &phase) const
{
    LOG4CPLUS_INFO(logger, stdsprintf("Writing phase %u to VFAT #%u of OH #%u.", phase, vfatN, ohN));
    // ohN check
    const uint32_t ohMax = readReg("GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // vfatN check
    if (vfatN >= oh::VFATS_PER_OH)
        EMIT_RPC_ERROR(stdsprintf("The vfatN parameter supplied (%u) exceeds the number of VFAT's per OH (%u).", vfatN, oh::VFATS_PER_OH), true);

    // phase check
    if (checkPhase(phase))
        return true;

    // Write the triplicated phase registers
    const uint32_t gbtN = gbt::elinkMappings::VFAT_TO_GBT[vfatN];
    LOG4CPLUS_INFO(logger, stdsprintf("Writing %u to the VFAT #%u phase of GBT #%u, on OH #%u.", phase, vfatN, gbtN, ohN));

    for (unsigned char regN = 0; regN < 3; regN++) {
        const uint16_t regAddress = elinkMappings::ELINK_TO_REGISTERS[elinkMappings::VFAT_TO_ELINK[vfatN]][regN];

        if (writeGBTRegLocal(ohN, gbtN, regAddress, phase))
            return true;
    }

    return false;

} //End writeGBTPhaseLocal

bool writeGBTRegLocal(const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value)
{
    // Check that the GBT exists
    if (gbtN >= GBTS_PER_OH)
        EMIT_RPC_ERROR(stdsprintf("The gbtN parameter supplied (%u) is larger than the number of GBT's per OH (%u).", gbtN, GBTS_PER_OH), true);

    // Check that the address is writable
    if (address >= CONFIG_SIZE)
        EMIT_RPC_ERROR(stdsprintf("GBT has %hu writable addresses while the provided address is %hu.", CONFIG_SIZE-1, address), true);

    // GBT registers are 8 bits long
    writeReg("GEM_AMC.SLOW_CONTROL.IC.READ_WRITE_LENGTH", 1);

    // Select the link number
    const uint32_t linkN = ohN*GBTS_PER_OH + gbtN;
    writeReg("GEM_AMC.SLOW_CONTROL.IC.GBTX_LINK_SELECT", linkN);

    // Write to the register
    writeReg("GEM_AMC.SLOW_CONTROL.IC.ADDRESS", address);
    writeReg("GEM_AMC.SLOW_CONTROL.IC.WRITE_DATA", value);
    writeReg("GEM_AMC.SLOW_CONTROL.IC.EXECUTE_WRITE", 1);

    return false;
} //End writeGBTRegLocal(...)

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
