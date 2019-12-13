#include "optohybrid.h"

#include "amc.h"
#include "hw_constants.h"

#include "xhal/common/rpc/register.h"

#include <chrono>
#include <fstream>
#include <thread>

namespace oh {
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
}

void oh::broadcastWrite::operator()(const uint32_t &ohN, const std::string &regName, const uint32_t &value, const uint32_t &mask) const
{
  uint32_t fw_maj = utils::readReg("GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  if (fw_maj == 1) {
    std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.Broadcast";

    std::string t_regName;
    t_regName = regBase + ".Reset";
    utils::writeRawReg(t_regName, 0);
    t_regName = regBase + ".Mask";
    utils::writeRawReg(t_regName, mask);
    t_regName = regBase + ".Request." + regName;
    utils::writeRawReg(t_regName, value);
    t_regName = regBase + ".Running";
    while (uint32_t t_res = utils::readRawReg(t_regName)) {
      if (t_res == 0xdeaddead)
        break;
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
  } else if (fw_maj == 3) {
    std::string t_regName;
    for (size_t vfatN = 0; vfatN < oh::VFATS_PER_OH; ++vfatN) {
      if (!((mask >> vfatN)&0x1)) {
        std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFAT" + std::to_string(vfatN) + ".";
        t_regName = regBase + regName;
        utils::writeReg(t_regName, value);
      }
    }
  } else {
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major: " << fw_maj);
  }
}

std::vector<uint32_t> oh::broadcastRead::operator()(const uint32_t &ohN, const std::string &regName, const uint32_t &mask) const
{
  uint32_t fw_maj = utils::readReg("GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.";
  if (fw_maj == 1) {
    regBase+="VFATS.VFAT";
  } else if (fw_maj == 3) {
    regBase+="VFAT";
  } else {
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major!");
  }

  std::string t_regName;
  std::vector<uint32_t> data;
  for (size_t i = 0; i < oh::VFATS_PER_OH; ++i) {
    if ((mask >> i)&0x1) {
      data.push_back(0);
    } else {
      t_regName = regBase + std::to_string(i) + "." + regName;

      try {
        data.push_back(utils::readReg(t_regName));
      } catch (...) { // FIXME add exception handling
        LOG4CPLUS_WARN(logger, "Error reading register " + t_regName);
        data.push_back(0xdeaddead); // FIXME do this right
      }
    }
  }
  return data;
}

void oh::biasAllVFATs::operator()(const uint32_t &ohN, const uint32_t &mask) const
{
  for (auto const& it : vfat_parameters) {
    broadcastWrite{}(ohN, it.first, it.second, mask);
  }
}

void oh::setAllVFATsToRunMode::operator()(const uint32_t &ohN, const uint32_t &mask) const
{
  switch (amc::fw_version_check("setAllVFATsToRunMode")) {
  case 3:
    broadcastWrite{}(ohN, "CFG_RUN", 0x1, mask);
    break;
  case 1:
    broadcastWrite{}(ohN, "ContReg0", 0x37, mask);
    break;
  default:
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major, do nothing");
    break;
  }

  return;
}

void oh::setAllVFATsToSleepMode::operator()(const uint32_t &ohN, const uint32_t &mask) const
{
  switch(amc::fw_version_check("setAllVFATsToRunMode")) {
  case 3:
    broadcastWrite{}(ohN, "CFG_RUN", 0x0, mask);
    break;
  case 1:
    broadcastWrite{}(ohN, "ContReg0", 0x36, mask);
    break;
  default:
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major, do nothing");
    break;
  }

  return;
}

void oh::loadTRIMDAC::operator()(const uint32_t &ohN, const std::string &configFile) const
{
  std::ifstream infile(configFile);

  if (!infile.is_open()) {
    std::stringstream errmsg;
    errmsg << "Could not open config file " << configFile;
    throw std::runtime_error(errmsg.str());
  }

  std::string line;
  uint32_t vfatN, vfatCH, trim, mask;
  std::getline(infile,line);
  while (std::getline(infile,line)) {
    std::stringstream iss(line);
    if (!(iss >> vfatN >> vfatCH >> trim >> mask)) {
      std::stringstream errmsg;
      errmsg << "Error reading settings";
      // throw std::runtime_error(errmsg.str());  // FIXME throw or break?
      LOG4CPLUS_ERROR(logger, errmsg.str());
      break;
    } else {
      std::string regName = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFATS.VFAT" + std::to_string(vfatN) + ".VFATChannels.ChanReg" + std::to_string(vfatCH);
      utils::writeRawReg(regName, trim + 32*mask);
    }
  }
}

void oh::stopCalPulse2AllChannels::operator()(const uint32_t &ohN, const uint32_t &mask, const uint32_t &chMin, const uint32_t &chMax) const
{
  const uint32_t fw_maj = utils::readReg("GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

  if (fw_maj == 1) {
    uint32_t trimVal = 0;
    for (size_t vfatN = 0; vfatN < oh::VFATS_PER_OH; ++vfatN) {
      if ((mask >> vfatN) & 0x1)
        continue;
      std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFATS.VFAT" + std::to_string(vfatN) + ".VFATChannels.ChanReg";
      for (uint32_t chan = chMin; chan <= chMax; ++chan) {
        if (chan > 127) {
          LOG4CPLUS_ERROR(logger, "OH " << ohN << ": Channel " << chan << " greater than the channel maximum " << 127);
        } else {
          trimVal = (0x3f & utils::readReg(regBase + std::to_string(chan)));
          utils::writeReg(regBase + std::to_string(chan), trimVal);
        }
      }
    }
  } else if (fw_maj == 3) {
    for (size_t vfatN = 0; vfatN < oh::VFATS_PER_OH; ++vfatN) {
      if ((mask >> vfatN) & 0x1)
        continue;
      std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFAT" + std::to_string(vfatN) + ".VFAT_CHANNELS.CHANNEL";
      for (uint32_t chan = chMin; chan <= chMax; ++chan) {
        utils::writeReg(regBase + std::to_string(chan) + ".CALPULSE_ENABLE", 0x0);
      }
    }
  } else {
    LOG4CPLUS_ERROR(logger, "Unexpected value for system release major: " << fw_maj);
  }

  return;
}

std::map<std::string, std::vector<uint32_t>> oh::statusOH::operator()(const uint32_t &ohMask) const
{
  LOG4CPLUS_INFO(logger, "Reeading OH status registers");

  std::array<std::string, 23 + 1> regs = {
    "CFG_PULSE_STRETCH", // FIXME seems like a copy/paste error?
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
    "CLOCKING.CLOCKING.LOGIC_MMCM_UNLOCKED_CNT"
  };

  std::map<std::string, std::vector<uint32_t>> statusValues;

  for (size_t ohN = 0; ohN < amc::OH_PER_AMC; ++ohN) {
    const std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".";
    for (auto &reg : regs) {
      if ((ohMask >> ohN) & 0x1) {
        statusValues[reg].push_back(utils::readReg(regBase + reg));
      } else {
        statusValues[reg].push_back(0xdeaddead);  // FIXME 0xdeaddead or 0x0?
      }
    }
  }

  return statusValues;
}

void oh::loadVT1::operator()(const uint32_t &ohN, const std::string &configFile, const uint32_t &vt1) const
{
  if (configFile != "") {
    LOG4CPLUS_INFO(logger, "Config file specified: " << configFile);
    std::ifstream infile(configFile);
    if (!infile.is_open()) {
      std::stringstream errmsg;
      errmsg << "Could not open config file " << configFile;
      throw std::runtime_error(errmsg.str());
    }

    std::string regBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFATS.VFAT";
    uint32_t vfatN, trimRange, vt1F;
    std::string line, regName;

    std::getline(infile,line);
    while (std::getline(infile,line)) {
      std::stringstream iss(line);
      if (!(iss >> vfatN >> vt1F >> trimRange)) {
        LOG4CPLUS_ERROR(logger, "Error reading settings");
        // throw std::runtime_error(errmsg.str());  // FIXME throw or break?
        break;
      } else {
        regName = regBase + std::to_string(vfatN) + ".VThreshold1";
        LOG4CPLUS_DEBUG(logger, "Writing " << std::hex << std::setw(4) << std::setfill('0') << vt1F << std::dec << " to : " << regName);
        utils::writeRawReg(regName, vt1F);
        regName = regBase + std::to_string(vfatN) + ".ContReg3";
        LOG4CPLUS_DEBUG(logger, "Writing " << std::hex << std::setw(4) << std::setfill('0') << trimRange << std::dec << " to : " << regName);
        utils::writeRawReg(regName, trimRange);
      }
    }
  } else {
    LOG4CPLUS_INFO(logger, "Config file not specified");
    LOG4CPLUS_DEBUG(logger, "Writing " << std::hex << std::setw(4) << std::setfill('0') << vt1 << std::dec << " to VThreshold1 of all VFATs");
    broadcastWrite{}(ohN, "VThreshold1", vt1);
  }
}

void oh::configureVFATs::operator()(const uint16_t &ohN, const std::string &trimConfigFile, const std::string &threshConfigFile, const uint8_t &vt1, const bool &setRunMode) const
{
  LOG4CPLUS_INFO(logger, "Bias VFATs");
  biasAllVFATs{}(ohN);
  LOG4CPLUS_INFO(logger, "Load VT1 settings to VFATs");
  loadVT1{}(ohN, threshConfigFile, vt1);
  LOG4CPLUS_INFO(logger, "Load trim settings to VFATs");
  loadTRIMDAC{}(ohN, trimConfigFile);
  if (setRunMode)
    setAllVFATsToRunMode{}(ohN);
}

void oh::configureScanModule::operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint32_t &scanmode, const bool &useUltra,
                                         const uint32_t &mask, const uint32_t &ch, const uint32_t &nevts, const uint32_t &dacMin,
                                         const uint32_t &dacMax, const uint32_t &dacStep) const
{
  std::string scanBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".ScanController";
  (useUltra) ? scanBase += ".ULTRA" : scanBase += ".THLAT";

  // check if another scan is running
  if (utils::readReg(scanBase + ".MONITOR.STATUS") > 0) {
    std::stringstream errmsg;
    errmsg << scanBase << ": Scan is already running, not starting a new scan";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  utils::writeRawReg(scanBase + ".RESET", 0x1);

  utils::writeReg(scanBase + ".CONF.MODE",   scanmode);
  if (useUltra) {
    utils::writeReg(scanBase + ".CONF.MASK", mask);
  } else {
    utils::writeReg(scanBase + ".CONF.CHIP", vfatN);
  }
  utils::writeReg(scanBase + ".CONF.CHAN",   ch);
  utils::writeReg(scanBase + ".CONF.NTRIGS", nevts);
  utils::writeReg(scanBase + ".CONF.MIN",    dacMin);
  utils::writeReg(scanBase + ".CONF.MAX",    dacMax);
  utils::writeReg(scanBase + ".CONF.STEP",   dacStep);

  return;
}

void oh::printScanConfiguration::operator()(const uint32_t &ohN, const bool &useUltra) const
{
  std::string scanBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".ScanController";
  (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

  std::map<std::string, uint32_t> regValues;

  // Set reg names
  regValues[scanBase + ".CONF.MODE"]      = 0;
  regValues[scanBase + ".CONF.MIN"]       = 0;
  regValues[scanBase + ".CONF.MAX"]       = 0;
  regValues[scanBase + ".CONF.STEP"]      = 0;
  regValues[scanBase + ".CONF.CHAN"]      = 0;
  regValues[scanBase + ".CONF.NTRIGS"]    = 0;
  regValues[scanBase + ".MONITOR.STATUS"] = 0;

  if (useUltra) {
    regValues[scanBase + ".CONF.MASK"]  = 0;
  } else {
    regValues[scanBase + ".CONF.CHIP"]  = 0;
  }

  LOG4CPLUS_INFO(logger, scanBase);
  for (auto & reg : regValues) {
    reg.second = utils::readReg(reg.first);
    LOG4CPLUS_INFO(logger, "FW " << reg.first << "   : " << reg.second);
    if (reg.second == 0xdeaddead) {
      std::stringstream errmsg;
      errmsg << "Error reading register " << reg.first;
      LOG4CPLUS_ERROR(logger, errmsg.str());
      throw std::runtime_error(errmsg.str());
    }
  }

  return;
}

void oh::startScanModule::operator()(const uint32_t &ohN, const bool &useUltra) const
{
  std::string scanBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".ScanController";
  (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

  // check if another scan is running
  if (utils::readReg(scanBase + ".MONITOR.STATUS") > 0) {
    std::stringstream errmsg;
    errmsg << scanBase << ": Scan is already running, not starting a new scan";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  // Check if there was an error in the config
  if (utils::readReg(scanBase + ".MONITOR.ERROR") > 0) {
    std::stringstream errmsg;
    errmsg << "OH " << ohN << ": Error in scan configuration, not starting a new scans";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  // Start the scan
  utils::writeReg(scanBase + ".START", 0x1);
  if (utils::readReg(scanBase + ".MONITOR.ERROR") || !(utils::readReg( scanBase + ".MONITOR.STATUS"))) {
    LOG4CPLUS_WARN(logger, "OH " << ohN << ": Scan failed to start");
    LOG4CPLUS_WARN(logger, "\tError code:\t " << utils::readReg(scanBase + ".MONITOR.ERROR"));
    LOG4CPLUS_WARN(logger, "\tStatus code:\t " << utils::readReg(scanBase + ".MONITOR.STATUS"));
  }

  return;
}

std::vector<uint32_t> oh::getUltraScanResults::operator()(const uint32_t &ohN, const uint32_t &nevts,
                                                          const uint32_t &dacMin, const uint32_t &dacMax, const uint32_t &dacStep) const
{
  std::string scanBase = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".ScanController.ULTRA";

  std::vector<uint32_t> outData;  // (oh::VFATS_PER_OH*(dacMax-dacMin+1)/dacStep)

  // Get L1A Count & num events
  uint32_t ohnL1A_0 = utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A");
  uint32_t ohnL1A   = utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A");
  uint32_t numtrigs = utils::readReg(scanBase + ".CONF.NTRIGS");

  // Print latency counts
  bool bIsLatency = false;
  if (utils::readReg(scanBase + ".CONF.MODE") == 2) {
    bIsLatency = true;

    LOG4CPLUS_INFO(logger, "At link " << ohN << ": "
                   << utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0 << "/"
                   << nevts*numtrigs << " L1As processed, "
                   << (utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0)*100./(nevts*numtrigs)
                   << "% done");
  }

  //Check if the scan is still running
  while (utils::readReg(scanBase + ".MONITOR.STATUS") > 0) {
    LOG4CPLUS_WARN(logger, "OH " << ohN << ": Ultra scan still running (0x"
                   << std::hex << std::setw(8) << std::setfill('0') << utils::readReg(scanBase + ".MONITOR.STATUS") << std::dec
                   << "), not returning results");
    if (bIsLatency) {
      if ((utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A") - ohnL1A) > numtrigs) {
        LOG4CPLUS_INFO(logger, "At link " << ohN << ": "
                       << utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0 << "/"
                       << nevts*numtrigs << " L1As processed, "
                       << (utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A") - ohnL1A_0)*100./(nevts*numtrigs)
                       << "% done");
        ohnL1A   =  utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".COUNTERS.T1.SENT.L1A");
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  LOG4CPLUS_DEBUG(logger, "OH " + std::to_string(ohN) + ": getUltraScanResults(...)");
  LOG4CPLUS_DEBUG(logger, "\tUltra scan status (0x"
                  << std::hex << std::setw(8) << std::setfill('0') << utils::readReg(scanBase + ".MONITOR.STATUS"));
  LOG4CPLUS_DEBUG(logger, "\tUltra scan results available (0x"
                  << std::hex << std::setw(6) << std::setfill('0') << utils::readReg(scanBase + ".MONITOR.READY"));

  for (uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep) {
    for (size_t vfatN = 0; vfatN < oh::VFATS_PER_OH; ++vfatN) {
      unsigned int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
      outData.push_back(utils::readReg("GEM_AMC.OH.OH" + std::to_string(ohN) + ".ScanController.ULTRA.RESULTS.VFAT" + std::to_string(vfatN)));
      LOG4CPLUS_DEBUG(logger, "\tUltra scan results: outData[" << idx << "] = (" << ((outData.at(idx)&0xff000000)>>24) << ", " << (outData.at(idx)&0xffffff) << ")");
    }
  }

  return outData;
}


extern "C" {
  const char *module_version_key = "optohybrid v1.0.1";
  const int module_activity_color = 4;

  void module_init(ModuleManager *modmgr)
  {
    utils::initLogging();

    if (memhub_open(&memsvc) != 0) {
      auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
      LOG4CPLUS_ERROR(logger, "Unable to connect to memory service: " << memsvc_get_last_error(memsvc));
      LOG4CPLUS_ERROR(logger, "Unable to load module");
      return;
    }

    xhal::common::rpc::registerMethod<oh::broadcastWrite>(modmgr);
    xhal::common::rpc::registerMethod<oh::broadcastRead>(modmgr);
    xhal::common::rpc::registerMethod<oh::biasAllVFATs>(modmgr);
    xhal::common::rpc::registerMethod<oh::setAllVFATsToRunMode>(modmgr);
    xhal::common::rpc::registerMethod<oh::setAllVFATsToSleepMode>(modmgr);
    xhal::common::rpc::registerMethod<oh::loadTRIMDAC>(modmgr);
    xhal::common::rpc::registerMethod<oh::statusOH>(modmgr);
    xhal::common::rpc::registerMethod<oh::stopCalPulse2AllChannels>(modmgr);
    xhal::common::rpc::registerMethod<oh::loadVT1>(modmgr);
    xhal::common::rpc::registerMethod<oh::configureVFATs>(modmgr);
    xhal::common::rpc::registerMethod<oh::configureScanModule>(modmgr);
    xhal::common::rpc::registerMethod<oh::getUltraScanResults>(modmgr);
    xhal::common::rpc::registerMethod<oh::printScanConfiguration>(modmgr);
    xhal::common::rpc::registerMethod<oh::startScanModule>(modmgr);
  }
}
