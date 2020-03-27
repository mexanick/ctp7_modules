#include "ctp7_modules/common/optohybrid.h"
#include "ctp7_modules/server/amc.h"
#include "ctp7_modules/common/hw_constants.h"
#include "ctp7_modules/common/utils.h"
#include "ctp7_modules/server/utils.h"
#include "ctp7_modules/common/vfat_parameters.h"

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
    xhal::common::rpc::registerMethod<oh::statusOH>(modmgr);
    xhal::common::rpc::registerMethod<oh::stopCalPulse2AllChannels>(modmgr);
  }
}
