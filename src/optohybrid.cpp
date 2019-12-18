#include "optohybrid.h"
#include "hw_constants.h"

#include "amc.h"
#include "utils.h"

void broadcastWriteLocal(localArgs * la, uint32_t ohN, std::string regName, uint32_t value, uint32_t mask) {
  uint32_t fw_maj = readReg(la, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  if (fw_maj == 1) {
    char regBase [100];
    sprintf(regBase, "GEM_AMC.OH.OH%i.GEB.Broadcast",ohN);

    std::string t_regName;

    //Reset broadcast module
    t_regName = std::string(regBase) + ".Reset";
    writeRawReg(la, t_regName, 0);
    //Set broadcast mask
    t_regName = std::string(regBase) + ".Mask";
    writeRawReg(la, t_regName, mask);
    //Issue broadcast write request
    t_regName = std::string(regBase) + ".Request." + regName;
    writeRawReg(la, t_regName, value);
    //Wait until broadcast write finishes
    t_regName = std::string(regBase) + ".Running";
    while (unsigned int t_res = readRawReg(la, t_regName))
    {
      if (t_res == 0xdeaddead) break;
      usleep(1000);
    }
  } else if (fw_maj == 3) {
    std::string t_regName;
    for (unsigned int vfatN=0; vfatN<oh::VFATS_PER_OH; vfatN++){
      if (!((mask >> vfatN)&0x1)) {
        char regBase [100];
        sprintf(regBase, "GEM_AMC.OH.OH%i.GEB.VFAT%i.",ohN, vfatN);
        t_regName = std::string(regBase)+regName;
        writeReg(la, t_regName, value);
      }
    }
  } else {
    LOG4CPLUS_ERROR(logger, stdsprintf("Unexpected value for system release major: %i",fw_maj));
  }
}

void broadcastWrite(const RPCMsg *request, RPCMsg *response) {
  GETLOCALARGS(response);

  std::string regName = request->get_string("reg_name");
  uint32_t value = request->get_word("value");
  uint32_t mask = request->get_key_exists("mask")?request->get_word("mask"):0xFF000000;
  uint32_t ohN = request->get_word("ohN");

  broadcastWriteLocal(&la, ohN, regName, value, mask);
  rtxn.abort();
}

void broadcastReadLocal(localArgs * la, uint32_t * outData, uint32_t ohN, std::string regName, uint32_t mask) {
  uint32_t fw_maj = readReg(la, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  char regBase [100];
  if (fw_maj == 1) {
    sprintf(regBase,"GEM_AMC.OH.OH%i.GEB.VFATS.VFAT",ohN);
  } else if (fw_maj == 3) {
    sprintf(regBase,"GEM_AMC.OH.OH%i.GEB.VFAT",ohN);
   } else {
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major!");
    la->response->set_string("error", "Unexpected value for system release major!");
  }
  std::string t_regName;
  for (unsigned int i=0; i<oh::VFATS_PER_OH; i++){
    if ((mask >> i)&0x1) outData[i] = 0;
    else {
      t_regName = std::string(regBase) + std::to_string(i)+"."+regName;
      outData[i] = readReg(la, t_regName);
      if (outData[i] == 0xdeaddead) la->response->set_string("error",stdsprintf("Error reading register %s",t_regName.c_str()));
    }
  }
  return;
}

void broadcastRead(const RPCMsg *request, RPCMsg *response) {
  GETLOCALARGS(response);

  std::string regName = request->get_string("reg_name");
  uint32_t mask = request->get_key_exists("mask")?request->get_word("mask"):0xFF000000;
  uint32_t ohN = request->get_word("ohN");

  uint32_t outData[oh::VFATS_PER_OH];
  broadcastReadLocal(&la, outData, ohN, regName, mask);
  response->set_word_array("data", outData, oh::VFATS_PER_OH);

  rtxn.abort();
}

// Set default values to VFAT parameters. VFATs will remain in sleep mode
void biasAllVFATsLocal(localArgs * la, uint32_t ohN, uint32_t mask) {
  for (auto & it:vfat_parameters)
  {
    broadcastWriteLocal(la, ohN, it.first, it.second, mask);
  }
}

void setAllVFATsToRunModeLocal(localArgs * la, uint32_t ohN, uint32_t mask) {
    switch(fw_version_check("setAllVFATsToRunMode", la)){
        case 3:
            broadcastWriteLocal(la, ohN, "CFG_RUN", 0x1, mask);
            break;
        case 1:
            broadcastWriteLocal(la, ohN, "ContReg0", 0x37, mask);
            break;
        default:
            LOG4CPLUS_ERROR(logger, "Unexpected value for system release major, do nothing");
            break;
    }

    return;
}

void setAllVFATsToSleepModeLocal(localArgs * la, uint32_t ohN, uint32_t mask) {
    switch(fw_version_check("setAllVFATsToRunMode", la)){
        case 3:
            broadcastWriteLocal(la, ohN, "CFG_RUN", 0x0, mask);
            break;
        case 1:
            broadcastWriteLocal(la, ohN, "ContReg0", 0x36, mask);
            break;
        default:
            LOG4CPLUS_ERROR(logger, "Unexpected value for system release major, do nothing");
            break;
    }

    return;
}

void loadVT1Local(localArgs * la, uint32_t ohN, std::string config_file, uint32_t vt1) {
  char regBase [100];
  sprintf(regBase,"GEM_AMC.OH.OH%i",ohN);
  uint32_t vfatN, trimRange;
  std::string line, regName;
  // Check if there's a config file. If yes, set the thresholds and trim range according to it, otherwise st only thresholds (equal on all chips) to provided vt1 value
  if (config_file!="") {
    LOG4CPLUS_INFO(logger, stdsprintf("CONFIG FILE FOUND: %s", config_file.c_str()));
    std::ifstream infile(config_file);
    std::getline(infile,line);// skip first line
    while (std::getline(infile,line))
    {
      std::stringstream iss(line);
      if (!(iss >> vfatN >> vt1 >> trimRange)) {
        LOG4CPLUS_ERROR(logger, "ERROR READING SETTINGS");
        la->response->set_string("error", "Error reading settings");
        break;
      } else {
        char regBase [100];
        sprintf(regBase,"GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VThreshold1",ohN, vfatN);
        //LOG4CPLUS_INFO(logger, stdsprintf("WRITING 0x%8x to REG: %s", vt1, regName.c_str()));
        writeRawReg(la, std::string(regName), vt1);
        sprintf(regBase,"GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.ContReg3",ohN, vfatN);
        //LOG4CPLUS_INFO(logger, stdsprintf("WRITING 0x%8x to REG: %s", trimRange, regName.c_str()));
        writeRawReg(la, regName, trimRange);
      }
    }
  } else {
    LOG4CPLUS_INFO(logger, "CONFIG FILE NOT FOUND");
    broadcastWriteLocal(la, ohN, "VThreshold1", vt1);
  }
}

void loadVT1(const RPCMsg *request, RPCMsg *response) {
  GETLOCALARGS(response);

  uint32_t ohN = request->get_word("ohN");
  std::string config_file = request->get_key_exists("thresh_config_filename")?request->get_string("thresh_config_filename"):"";
  uint32_t vt1 = request->get_key_exists("vt1")?request->get_word("vt1"):0x64;

  loadVT1Local(&la, ohN, config_file, vt1);

  rtxn.abort();
}

void loadTRIMDACLocal(localArgs * la, uint32_t ohN, std::string config_file) {
  std::ifstream infile(config_file);
  std::string line, regName;
  uint32_t vfatN, vfatCH, trim, mask;
  std::getline(infile,line);// skip first line
  while (std::getline(infile,line))
  {
    std::stringstream iss(line);
    if (!(iss >> vfatN >> vfatCH >> trim >> mask)) {
		  LOG4CPLUS_ERROR(logger, "ERROR READING SETTINGS");
      la->response->set_string("error", "Error reading settings");
      break;
    } else {
      char regBase [100];
      sprintf(regBase,"GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN, vfatN, vfatCH);
      regName = std::string(regBase);
      writeRawReg(la, regName, trim + 32*mask);
    }
  }
}

void loadTRIMDAC(const RPCMsg *request, RPCMsg *response) {
  GETLOCALARGS(response);

  uint32_t ohN = request->get_word("ohN");
  std::string config_file = request->get_string("trim_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";

  loadTRIMDACLocal(&la, ohN, config_file);
  rtxn.abort();
}

void configureVFATs(const RPCMsg *request, RPCMsg *response) {
  GETLOCALARGS(response);

  uint32_t ohN = request->get_word("ohN");
  std::string trim_config_file = request->get_string("trim_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
  std::string thresh_config_file = request->get_key_exists("thresh_config_filename")?request->get_string("thresh_config_filename"):"";
  uint32_t vt1 = request->get_key_exists("vt1")?request->get_word("vt1"):0x64;

  LOG4CPLUS_INFO(logger, "BIAS VFATS");
  biasAllVFATsLocal(&la, ohN);
  LOG4CPLUS_INFO(logger, "LOAD VT1 VFATS");
  loadVT1Local(&la, ohN, thresh_config_file, vt1);
  LOG4CPLUS_INFO(logger, "LOAD TRIM VFATS");
  loadTRIMDACLocal(&la, ohN, trim_config_file);
  if (request->get_key_exists("set_run")) setAllVFATsToRunModeLocal(&la, ohN);

  rtxn.abort();
}

void configureScanModuleLocal(localArgs * la, uint32_t ohN, uint32_t vfatN, uint32_t scanmode, bool useUltra, uint32_t mask, uint32_t ch, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep){
    /*
     *     Configure the firmware scan controller
     *      mode: 0 Threshold scan
     *            1 Threshold scan per channel
     *            2 Latency scan
     *            3 s-curve scan
     *            4 Threshold scan with tracking data
     *      vfat: for single VFAT scan, specify the VFAT number
     *            for ULTRA scan, specify the VFAT mask
     */

    std::stringstream sstream;
    sstream<<ohN;
    std::string strOhN = sstream.str();

    //Set Scan Base
    std::string scanBase = "GEM_AMC.OH.OH" + strOhN + ".ScanController";
    (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

    // check if another scan is running
    if (readReg(la, scanBase + ".MONITOR.STATUS") > 0) {
      LOG4CPLUS_WARN(logger, stdsprintf("%s: Scan is already running, not starting a new scan", scanBase.c_str()));
      la->response->set_string("error", "Scan is already running, not starting a new scan");
      return;
    }

    // reset scan module
    writeRawReg(la, scanBase + ".RESET", 0x1);

    // write scan parameters
    writeReg(la, scanBase + ".CONF.MODE", scanmode);
    if (useUltra){
        writeReg(la, scanBase + ".CONF.MASK", mask);
    }
    else{
        writeReg(la, scanBase + ".CONF.CHIP", vfatN);
    }
    writeReg(la, scanBase + ".CONF.CHAN", ch);
    writeReg(la, scanBase + ".CONF.NTRIGS", nevts);
    writeReg(la, scanBase + ".CONF.MIN", dacMin);
    writeReg(la, scanBase + ".CONF.MAX", dacMax);
    writeReg(la, scanBase + ".CONF.STEP", dacStep);

    return;
} //End configureScanModuleLocal(...)

void configureScanModule(const RPCMsg *request, RPCMsg *response){
    /*
     *     Configure the firmware scan controller
     *      mode: 0 Threshold scan
     *            1 Threshold scan per channel
     *            2 Latency scan
     *            3 s-curve scan
     *            4 Threshold scan with tracking data
     *      vfat: for single VFAT scan, specify the VFAT number
     *            for ULTRA scan, specify the VFAT mask
     */

    GETLOCALARGS(response);

    //Get OH and scanmode
    uint32_t ohN = request->get_word("ohN");
    uint32_t scanmode = request->get_word("scanmode");

    //Setup ultra mode, mask, and/or vfat number
    bool useUltra = false;
    uint32_t mask = 0xFFFFFFFF;
    uint32_t vfatN = 0;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
        mask = request->get_word("mask");
    }
    else{
        vfatN = request->get_word("vfatN");
    }

    uint32_t ch = request->get_word("ch");
    uint32_t nevts = request->get_word("nevts");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");


    configureScanModuleLocal(&la, ohN, vfatN, scanmode, useUltra, mask, ch, nevts, dacMin, dacMax, dacStep);

    rtxn.abort();
} //End configureScanModule(...)

void printScanConfigurationLocal(localArgs * la, uint32_t ohN, bool useUltra){
    std::stringstream sstream;
    sstream<<ohN;
    std::string strOhN = sstream.str();

    //Set Scan Base
    std::string scanBase = "GEM_AMC.OH.OH" + strOhN + ".ScanController";
    (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

    //char regBuf[200];
    //sprintf(regBuf,scanBase);

    std::map<std::string, uint32_t> map_regValues;

    //Set reg names
    map_regValues[scanBase + ".CONF.MODE"] = 0;
    map_regValues[scanBase + ".CONF.MIN"] = 0;
    map_regValues[scanBase + ".CONF.MAX"] = 0;
    map_regValues[scanBase + ".CONF.STEP"] = 0;
    map_regValues[scanBase + ".CONF.CHAN"] = 0;
    map_regValues[scanBase + ".CONF.NTRIGS"] = 0;
    map_regValues[scanBase + ".MONITOR.STATUS"] = 0;

    if (useUltra){
        map_regValues[scanBase + ".CONF.MASK"] = 0;
    }
    else{
        map_regValues[scanBase + ".CONF.CHIP"] = 0;
    }

    stdsprintf(scanBase.c_str());
    for(auto regIter = map_regValues.begin(); regIter != map_regValues.end(); ++regIter){
        (*regIter).second = readReg(la, (*regIter).first);
        stdsprintf("FW %s   : %d",(*regIter).first.c_str(), (*regIter).second);
        if ( (*regIter).second == 0xdeaddead) la->response->set_string("error",stdsprintf("Error reading register %s", (*regIter).first.c_str()));
    }

    return;
} //End printScanConfigurationLocal(...)

void printScanConfiguration(const RPCMsg *request, RPCMsg *response){
    GETLOCALARGS(response);
    
    uint32_t ohN = request->get_word("ohN");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }

    printScanConfigurationLocal(&la, ohN, useUltra);

    rtxn.abort();
} //End printScanConfiguration(...)

void startScanModuleLocal(localArgs * la, uint32_t ohN, bool useUltra){
    std::stringstream sstream;
    sstream<<ohN;
    std::string strOhN = sstream.str();

    //Set Scan Base
    std::string scanBase = "GEM_AMC.OH.OH" + strOhN + ".ScanController";
    (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

    // check if another scan is running
    if (readReg(la, scanBase + ".MONITOR.STATUS") > 0) {
      LOG4CPLUS_WARN(logger, stdsprintf("%s: Scan is already running, not starting a new scan", scanBase.c_str()));
      la->response->set_string("error", "Scan is already running, not starting a new scan");
      return;
    }

    //Check if there was an error in the config
    if (readReg(la, scanBase + ".MONITOR.ERROR") > 0 ){
        LOG4CPLUS_WARN(logger, stdsprintf("OH %i: Error in scan configuration, not starting a new scans",ohN));
        la->response->set_string("error","Error in scan configuration");
        return;
    }

    //Start the scan
    writeReg(la, scanBase + ".START", 0x1);
    if (readReg(la, scanBase + ".MONITOR.ERROR")
            || !(readReg(la,  scanBase + ".MONITOR.STATUS")))
    {
        LOG4CPLUS_WARN(logger, stdsprintf("OH %i: Scan failed to start",ohN));
        LOG4CPLUS_WARN(logger, stdsprintf("\tERROR Code:\t %i",readReg(la, scanBase + ".MONITOR.ERROR")));
        LOG4CPLUS_WARN(logger, stdsprintf("\tSTATUS Code:\t %i",readReg(la, scanBase + ".MONITOR.STATUS")));
    }

    return;
} //End startScanModuleLocal(...)

void startScanModule(const RPCMsg *request, RPCMsg *response){
    GETLOCALARGS(response);
    
    uint32_t ohN = request->get_word("ohN");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }

    startScanModuleLocal(&la, ohN, useUltra);

    rtxn.abort();
} //End startScanModule(...)

void getUltraScanResultsLocal(localArgs * la, uint32_t *outData, uint32_t ohN, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep){
    std::stringstream sstream;
    sstream<<ohN;
    std::string strOhN = sstream.str();

    //Set Scan Base
    std::string scanBase = "GEM_AMC.OH.OH" + strOhN + ".ScanController.ULTRA";

    //Get L1A Count & num events
    uint32_t ohnL1A_0 =  readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A");
    uint32_t ohnL1A   =  readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A");
    uint32_t numtrigs = readReg(la, scanBase + ".CONF.NTRIGS");

    //Print latency counts
    bool bIsLatency = false;
    if( readReg(la, scanBase + ".CONF.MODE") == 2){
        bIsLatency = true;

        stdsprintf(
                "At Link %i: %d/%d L1As processed, %d%% done",
                    ohN,
                    readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0,
                    nevts*numtrigs,
                    (readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0)*100./(nevts*numtrigs)
                );
    }

    //Check if the scan is still running
    while(readReg(la, scanBase + ".MONITOR.STATUS") > 0){
        stdsprintf("OH %i: Ultra scan still running (0x%x), not returning results",ohN,
                    readReg(la, scanBase + ".MONITOR.STATUS"));
        if (bIsLatency){
            if( (readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A") - ohnL1A ) > numtrigs){
                stdsprintf(
                        "At Link %i: %d/%d L1As processed, %d%% done",
                            ohN,
                            readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0,
                            nevts*numtrigs,
                            (readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0)*100./(nevts*numtrigs)
                        );
                ohnL1A   =  readReg(la, "GEM_AMC.OH.OH" + strOhN + ".COUNTERS.T1.SENT.L1A");
            }
        }
        sleep(0.1);
    }

    LOG4CPLUS_DEBUG(logger, "OH " + strOhN + ": getUltraScanResults(...)");
    LOG4CPLUS_DEBUG(logger, stdsprintf("\tUltra scan status (0x%08x)\n",readReg(la, scanBase + ".MONITOR.STATUS")));
    LOG4CPLUS_DEBUG(logger, stdsprintf("\tUltra scan results available (0x%06x)",readReg(la, scanBase + ".MONITOR.READY")));

    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
        for(unsigned int vfatN = 0; vfatN < oh::VFATS_PER_OH; ++vfatN){
            unsigned int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
            outData[idx] = readReg(la, stdsprintf("GEM_AMC.OH.OH%i.ScanController.ULTRA.RESULTS.VFAT%i",ohN,vfatN));
            LOG4CPLUS_DEBUG(logger, stdsprintf("\tUltra scan results: outData[%i] = (%i, %i)",idx,(outData[idx]&0xff000000)>>24,(outData[idx]&0xffffff)));
        }
    }

    return;
} //End getUltraScanResultsLocal(...)

void getUltraScanResults(const RPCMsg *request, RPCMsg *response){
    GETLOCALARGS(response);
    
    uint32_t ohN = request->get_word("ohN");
    uint32_t nevts = request->get_word("nevts");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");

    uint32_t outData[oh::VFATS_PER_OH*(dacMax-dacMin+1)/dacStep];
    getUltraScanResultsLocal(&la, outData, ohN, nevts, dacMin, dacMax, dacStep);
    response->set_word_array("data",outData,oh::VFATS_PER_OH*(dacMax-dacMin+1)/dacStep);

    rtxn.abort();
} //End getUltraScanResults(...)

void stopCalPulse2AllChannelsLocal(localArgs *la, uint32_t ohN, uint32_t mask, uint32_t ch_min, uint32_t ch_max){
    //Get FW release
    uint32_t fw_maj = readReg(la, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (fw_maj == 1) {
        uint32_t trimVal=0;
        for (unsigned int vfatN=0; vfatN<oh::VFATS_PER_OH; ++vfatN) {
            if ((mask >> vfatN) & 0x1) continue; //skip masked VFATs
            for (uint32_t chan=ch_min; chan<=ch_max; ++chan) {
                if (chan>127) {
                    LOG4CPLUS_ERROR(logger, stdsprintf("OH %d: Chan %d greater than possible chan_max %d",ohN,chan,127));
                } else {
                    trimVal = (0x3f & readReg(la, stdsprintf("GEM_AMC.OH.OH%d.GEB.VFATS.VFAT%d.VFATChannels.ChanReg%d",ohN,vfatN,chan)));
                    writeReg(la, stdsprintf("GEM_AMC.OH.OH%d.GEB.VFATS.VFAT%d.VFATChannels.ChanReg%d",ohN,vfatN,chan),trimVal);
                }
            }
        }
    } else if (fw_maj == 3) {
        for (unsigned int vfatN = 0; vfatN < oh::VFATS_PER_OH; vfatN++) {
            if ((mask >> vfatN) & 0x1) continue; //skip masked VFATs
            for (uint32_t chan=ch_min; chan<=ch_max; ++chan) {
                writeReg(la, stdsprintf("GEM_AMC.OH.OH%d.GEB.VFAT%d.VFAT_CHANNELS.CHANNEL%d.CALPULSE_ENABLE", ohN, vfatN, chan), 0x0);
            }
        }
    } else {
        LOG4CPLUS_ERROR(logger, stdsprintf("Unexpected value for system release major: %i",fw_maj));
    }

    return;
}

void stopCalPulse2AllChannels(const RPCMsg *request, RPCMsg *response){
    GETLOCALARGS(response);
    
    uint32_t ohN = request->get_word("ohN");
    uint32_t mask = request->get_word("mask");
    uint32_t ch_min = request->get_word("ch_min");
    uint32_t ch_max = request->get_word("ch_max");

    stopCalPulse2AllChannelsLocal(&la, ohN, mask, ch_min, ch_max);

    rtxn.abort();
}

void statusOHLocal(localArgs * la, uint32_t ohEnMask){
    std::string regs [] = {"CFG_PULSE_STRETCH ",
                           "TRIG.CTRL.SBIT_SOT_READY",
                           "TRIG.CTRL.SBIT_SOT_UNSTABLE",
                           "GBT.TX.TX_READY",
                           "GBT.RX.RX_READY",
                           "GBT.RX.RX_VALID",
                           "GBT.RX.CNT_LINK_ERR",
                           "ADC.CTRL.CNT_OVERTEMP",
                           "ADC.CTRL.CNT_VCCAUX_ALARM",
                           "ADC.CTRL.CNT_VCCINT_ALARM",
                           "CONTROL.RELEASE.DATE",
                           "CONTROL.RELEASE.VERSION.MAJOR",
                           "CONTROL.RELEASE.VERSION.MINOR",
                           "CONTROL.RELEASE.VERSION.BUILD",
                           "CONTROL.RELEASE.VERSION.GENERATION",
                           "CONTROL.SEM.CNT_SEM_CRITICAL",
                           "CONTROL.SEM.CNT_SEM_CORRECTION",
                           "TRIG.CTRL.SOT_INVERT",
                           "GBT.TX.CNT_RESPONSE_SENT",
                           "GBT.RX.CNT_REQUEST_RECEIVED",
                           "CLOCKING.CLOCKING.GBT_MMCM_LOCKED",
                           "CLOCKING.CLOCKING.LOGIC_MMCM_LOCKED",
                           "CLOCKING.CLOCKING.GBT_MMCM_UNLOCKED_CNT",
                           "CLOCKING.CLOCKING.LOGIC_MMCM_UNLOCKED_CNT"};
    std::string regName;

    for(unsigned int ohN = 0; ohN < amc::OH_PER_AMC; ohN++) if((ohEnMask >> ohN) & 0x1)

    {
        char regBase [100];
        sprintf(regBase, "GEM_AMC.OH.OH%i.",ohN);
        for (auto &reg : regs) {
            regName = std::string(regBase)+reg;
            la->response->set_word(regName,readReg(la,regName));
        }
    }
}

void statusOH(const RPCMsg *request, RPCMsg *response)
{
    GETLOCALARGS(response);

    uint32_t ohEnMask = request->get_word("ohEnMask");
    LOG4CPLUS_INFO(logger, "Reeading OH status");

    statusOHLocal(&la, ohEnMask);
    rtxn.abort();
}

extern "C" {
    const char *module_version_key = "optohybrid v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        initLogging();

        if (memhub_open(&memsvc) != 0) {
            auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
            LOG4CPLUS_ERROR(logger, LOG4CPLUS_TEXT("Unable to connect to memory service: ") << memsvc_get_last_error(memsvc));
            LOG4CPLUS_ERROR(logger, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }

        modmgr->register_method("optohybrid", "broadcastRead", broadcastRead);
        modmgr->register_method("optohybrid", "broadcastWrite", broadcastWrite);
        modmgr->register_method("optohybrid", "configureScanModule", configureScanModule);
        modmgr->register_method("optohybrid", "configureVFATs", configureVFATs);
        modmgr->register_method("optohybrid", "getUltraScanResults", getUltraScanResults);
        modmgr->register_method("optohybrid", "loadTRIMDAC", loadTRIMDAC);
        modmgr->register_method("optohybrid", "loadVT1", loadVT1);
        modmgr->register_method("optohybrid", "printScanConfiguration", printScanConfiguration);
        modmgr->register_method("optohybrid", "startScanModule", startScanModule);
        modmgr->register_method("optohybrid", "stopCalPulse2AllChannels", stopCalPulse2AllChannels);
        modmgr->register_method("optohybrid", "statusOH", statusOH);
    }
}
