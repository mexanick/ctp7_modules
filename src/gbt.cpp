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

void scanGBTPhases(const RPCMsg *request, RPCMsg *response)
{
    GETLOCALARGS(response);

    // Get the keys
    const uint32_t ohN = request->get_word("ohN");
    const uint32_t nScans = request->get_word("nScans");
    const uint8_t phaseMin = request->get_word("phaseMin");
    const uint8_t phaseMax = request->get_word("phaseMax");
    const uint8_t phaseStep = request->get_word("phaseStep");

    // Perform the scan
    LOGGER->log_message(LogManager::INFO, stdsprintf("Calling Local Method for OH #%u.", ohN));
    if (scanGBTPhasesLocal(&la, ohN, nScans, phaseMin, phaseMax, phaseStep)) {
        LOGGER->log_message(LogManager::INFO, stdsprintf("GBT Scan for OH #%u Failed.", ohN));
        rtxn.abort();
    }

    rtxn.abort();
} //Enc scanGBTPhase

bool scanGBTPhasesLocal(localArgs *la, const uint32_t ohN, const uint32_t N, const uint8_t phaseMin, const uint8_t phaseMax, const uint8_t phaseStep)
{
    LOGGER->log_message(LogManager::INFO, stdsprintf("Scanning the phases for OH #%u.", ohN));

    // ohN check
    const uint32_t ohMax = readReg(la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(la->response, stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // phaseMin check
    if (gbt::checkPhase(la->response, phaseMin))
        return true;

    // phaseMax check
    if (gbt::checkPhase(la->response, phaseMax))
        return true;

    // Results array
    std::vector<std::vector<uint32_t>> results(oh::VFATS_PER_OH, std::vector<uint32_t>(16));

    // Perform the scan
    for (uint8_t phase = phaseMin; phase <= phaseMax; phase += phaseStep) {
        // Set the new phases
        for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
            if (writeGBTPhaseLocal(la, ohN, vfatN, phase))
                return true;
        }

        // Wait for the phases to be set
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        for (uint32_t repN = 0; repN < N; repN++) {
            // Try to synchronize the VFAT's
            writeReg(la, "GEM_AMC.GEM_SYSTEM.CTRL.LINK_RESET", 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Check the VFAT status
            slowCtrlErrCntVFAT vfatErrs;
            for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
                // check SYNC_ERR_CNT
                if (readReg(la, stdsprintf("GEM_AMC.OH_LINKS.OH%hu.VFAT%hu.SYNC_ERR_CNT", ohN, vfatN)) != 0){
                    continue;
                }

                // check CFG_RUN
                vfatErrs = repeatedRegReadLocal(la, stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.CFG_RUN", ohN, vfatN), true, 10);
                if (vfatErrs.sum != 0){
                    continue;
                }

                // check HW_ID_VER
                vfatErrs = repeatedRegReadLocal(la, stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.HW_ID_VER", ohN, vfatN), true, 10);
                if (vfatErrs.sum != 0){
                    continue;
                }

                // check HW_ID
                vfatErrs = repeatedRegReadLocal(la, stdsprintf("GEM_AMC.OH.OH%hu.GEB.VFAT%hu.HW_ID", ohN, vfatN), true, 10);
                if (vfatErrs.sum != 0){
                    continue;
                }

                //if (syncErrCnt && cfgRun && hwID && hwIDVer)
                //    results[vfatN][phase]++;
                results[vfatN][phase]++;
            }
        }
    }

    // Write the results to RPC keys
    for (uint32_t vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
        la->response->set_word_array(stdsprintf("OH%u.VFAT%u", ohN, vfatN), results[vfatN]);
    }

    return false;
} //End scanGBTPhaseLocal

void writeGBTConfig(const RPCMsg *request, RPCMsg *response)
{
    GETLOCALARGS(response);

    // Get the keys
    const uint32_t ohN = request->get_word("ohN");
    const uint32_t gbtN = request->get_word("gbtN");

    // We must check the size of the config key
    const uint32_t configSize = request->get_binarydata_size("config");
    if (configSize != gbt::CONFIG_SIZE)
        EMIT_RPC_ERROR(response, stdsprintf("The provided configuration has not the correct size. It is %u registers long while this methods expects %hu 8-bits registers.", configSize, gbt::CONFIG_SIZE), (void)"");

    gbt::config_t config{};
    request->get_binarydata("config", config.data(), config.size());

    // Write the configuration
    writeGBTConfigLocal(&la, ohN, gbtN, config);

    rtxn.abort();
} //End writeGBTConfig(...)

bool writeGBTConfigLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const gbt::config_t &config)
{
    LOGGER->log_message(LogManager::INFO, stdsprintf("Writing the configuration of OH #%u - GBTX #%u.", ohN, gbtN));

    // ohN check
    const uint32_t ohMax = readReg(la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(la->response, stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // gbtN check
    if (gbtN >= gbt::GBTS_PER_OH)
        EMIT_RPC_ERROR(la->response, stdsprintf("The gbtN parameter supplied (%u) exceeds the number of GBT's per OH (%u).", gbtN, gbt::GBTS_PER_OH), true);

    // Write all the registers
    for (size_t address = 0; address < gbt::CONFIG_SIZE; address++) {
        if (writeGBTRegLocal(la, ohN, gbtN, static_cast<uint16_t>(address), config[address]))
            return true;
    }

    return false;
} //End writeGBTConfigLocal(...)

void writeGBTPhase(const RPCMsg *request, RPCMsg *response)
{
    GETLOCALARGS(response);

    // Get the keys
    const uint32_t ohN = request->get_word("ohN");
    const uint32_t vfatN = request->get_word("vfatN");
    const uint8_t phase = request->get_word("phase");

    // Write the phase
    writeGBTPhaseLocal(&la, ohN, vfatN, phase);

    rtxn.abort();
} //End writeGBTPhase

bool writeGBTPhaseLocal(localArgs *la, const uint32_t ohN, const uint32_t vfatN, const uint8_t phase)
{
    //LOGGER->log_message(LogManager::INFO, stdsprintf("Writing %u to the VFAT #%u phase of OH #%u.", phase, vfatN, ohN));

    // ohN check
    const uint32_t ohMax = readReg(la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(la->response, stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true);

    // vfatN check
    if (vfatN >= oh::VFATS_PER_OH)
        EMIT_RPC_ERROR(la->response, stdsprintf("The vfatN parameter supplied (%u) exceeds the number of VFAT's per OH (%u).", vfatN, oh::VFATS_PER_OH), true);

    // phase check
    if (gbt::checkPhase(la->response, phase))
        return true;

    // Write the triplicated phase registers
    const uint32_t gbtN = gbt::elinkMappings::VFAT_TO_GBT[vfatN];
    LOGGER->log_message(LogManager::INFO, stdsprintf("Writing %u to the VFAT #%u phase of GBT #%u, on OH #%u.", phase, vfatN, gbtN, ohN));

    for (unsigned char regN = 0; regN < 3; regN++) {
        const uint16_t regAddress = gbt::elinkMappings::ELINK_TO_REGISTERS[gbt::elinkMappings::VFAT_TO_ELINK[vfatN]][regN];

        if (writeGBTRegLocal(la, ohN, gbtN, regAddress, phase))
            return true;
    }

    return false;
} //End writeGBTPhaseLocal

bool writeGBTRegLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value)
{
    // Check that the GBT exists
    if (gbtN >= gbt::GBTS_PER_OH)
        EMIT_RPC_ERROR(la->response, stdsprintf("The gbtN parameter supplied (%u) is larger than the number of GBT's per OH (%u).", gbtN, gbt::GBTS_PER_OH), true);

    // Check that the address is writable
    if (address >= gbt::CONFIG_SIZE)
        EMIT_RPC_ERROR(la->response, stdsprintf("GBT has %hu writable addresses while the provided address is %hu.", gbt::CONFIG_SIZE-1, address), true);

    // GBT registers are 8 bits long
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.READ_WRITE_LENGTH", 1);

    // Select the link number
    const uint32_t linkN = ohN*gbt::GBTS_PER_OH + gbtN;
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.GBTX_LINK_SELECT", linkN);

    // Write to the register
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.ADDRESS", address);
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.WRITE_DATA", value);
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.EXECUTE_WRITE", 1);

    return false;
} //End writeGBTRegLocal(...)

extern "C" {
    const char *module_version_key = "gbt v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memhub_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("gbt", "writeGBTConfig", writeGBTConfig);
        modmgr->register_method("gbt", "writeGBTPhase", writeGBTPhase);
        modmgr->register_method("gbt", "scanGBTPhases", scanGBTPhases);
    }
}
