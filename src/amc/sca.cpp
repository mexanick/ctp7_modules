/*!
 * \file amc/sca.cpp
 * \brief AMC SCA methods for RPC modules
 */

#include "amc/sca.h"
#include "hw_constants.h"

#include <sstream>
#include <string>
#include <vector>

namespace amc {
  namespace sca {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
  }
}

uint32_t amc::sca::formatSCAData(uint32_t const& data)
{
  return (
          ((data&0xff000000)>>24) +
          ((data>>8)&0x0000ff00)  +
          ((data&0x0000ff00)<<8)  +
          ((data&0x000000ff)<<24)
          );
}

void amc::sca::sendSCACommand::operator()(uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask) const
{
  // FIXME: DECIDE WHETHER TO HAVE HERE // if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
  // FIXME: DECIDE WHETHER TO HAVE HERE //   utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",         0xffffffff);
  // FIXME: DECIDE WHETHER TO HAVE HERE // }

  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.LINK_ENABLE_MASK",       ohMask);
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_CHANNEL",ch);
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_COMMAND",cmd);
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_LENGTH", len);
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_DATA",   formatSCAData(data));
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_EXECUTE",0x1);
}

std::vector<uint32_t> amc::sca::sendSCACommandWithReply::operator()(uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask) const
{
  // FIXME: DECIDE WHETHER TO HAVE HERE // if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
  // FIXME: DECIDE WHETHER TO HAVE HERE //   uint32_t monMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  // FIXME: DECIDE WHETHER TO HAVE HERE //   utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);
  // FIXME: DECIDE WHETHER TO HAVE HERE // }

  amc::sca::sendSCACommand{}(ch, cmd, len, data, ohMask);

  // read reply from OptoHybrids attached to an AMC
  std::vector<uint32_t> reply;
  reply.reserve(amc::OH_PER_AMC);
  for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
    if ((ohMask >> oh) & 0x1) {
      std::stringstream regName;
      regName << "GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_REPLY_OH"
              << oh << ".SCA_RPY_DATA";
      reply.push_back(formatSCAData(utils::readReg(regName.str())));
    } else {
      // FIXME Sensible null value?
      reply.push_back(0);
    }
  }
  // FIXME: DECIDE WHETHER TO HAVE HERE // utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  return reply;
}

std::vector<uint32_t> amc::sca::scaCTRLCommand::operator()(SCACTRLCommandT const& cmd, uint16_t const& ohMask, uint8_t const& len, uint32_t const& data) const
{
  uint32_t monMask = 0xffffffff;
  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    monMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);
  }

  std::vector<uint32_t> result;
  switch (cmd) {
  case SCACTRLCommand::CTRL_R_ID_V2:
    result = amc::sca::sendSCACommandWithReply{}(0x14, cmd, 0x1, 0x1, ohMask);
  case SCACTRLCommand::CTRL_R_ID_V1:
    result = amc::sca::sendSCACommandWithReply{}(0x14, cmd, 0x1, 0x1, ohMask);
  case SCACTRLCommand::CTRL_R_SEU:
    result = amc::sca::sendSCACommandWithReply{}(0x13, cmd, 0x1, 0x0, ohMask);
  case SCACTRLCommand::CTRL_C_SEU:
    result = amc::sca::sendSCACommandWithReply{}(0x13, cmd, 0x1, 0x0, ohMask);
  case SCACTRLCommand::CTRL_W_CRB:
    amc::sca::sendSCACommand{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_W_CRC:
    amc::sca::sendSCACommand{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_W_CRD:
    amc::sca::sendSCACommand{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRB:
    result = amc::sca::sendSCACommandWithReply{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRC:
    result = amc::sca::sendSCACommandWithReply{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRD:
    result = amc::sca::sendSCACommandWithReply{}(SCAChannel::CTRL, cmd, len, data, ohMask);
  default:
    // maybe don't do this by default, return error or invalid option?
    result = amc::sca::sendSCACommandWithReply{}(SCAChannel::CTRL, SCACTRLCommand::GET_DATA, len, data, ohMask);
  }

  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  }

  return result;
}

std::vector<uint32_t> amc::sca::scaI2CCommand::operator()(SCAI2CChannelT const& ch, SCAI2CCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask) const
{

  // enable the I2C bus through the CTRL CR{B,C,D} registers
  // 16 I2C channels available
  // 4 I2C modes of communication
  // I2C frequency selection
  // Allows RMW transactions

  std::vector<uint32_t> result;

  uint32_t monMask = 0xffffffff;
  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    monMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);
  }

  amc::sca::sendSCACommand{}(ch, cmd, len, data, ohMask);

  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  }

  return result;
}

std::vector<uint32_t> amc::sca::scaGPIOCommand::operator()(SCAGPIOCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask) const
{
  // enable the GPIO bus through the CTRL CRB register, bit 2
  uint32_t monMask = 0xffffffff;
  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    monMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);
  }

  std::vector<uint32_t> reply = amc::sca::sendSCACommandWithReply{}(SCAChannel::GPIO, cmd, len, data, ohMask);

  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  }

  return reply;
}

std::vector<uint32_t> amc::sca::scaADCCommand::operator()(SCAADCChannelT const& ch, uint16_t const& ohMask) const
{
  uint32_t monMask = 0xffffffff;
  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    monMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);
  }

  // enable the ADC bus through the CTRL CRD register, bit 4
  // select the ADC channel
  amc::sca::sendSCACommand{}(SCAChannel::ADC, SCAADCCommand::ADC_W_MUX, 0x4, ch, ohMask);

  // enable the current sink if reading the temperature ADCs
  if (SCAADCChannel::useCurrentSource(ch))
    sendSCACommand{}(SCAChannel::ADC, SCAADCCommand::ADC_W_CURR, 0x4, 0x1<<ch, ohMask);
  // start the conversion and get the result
  std::vector<uint32_t> result = amc::sca::sendSCACommandWithReply{}(SCAChannel::ADC, SCAADCCommand::ADC_GO, 0x4, 0x1, ohMask);

  // disable the current sink if reading the temperature ADCs
  if (SCAADCChannel::useCurrentSource(ch))
    sendSCACommand{}(SCAChannel::ADC, SCAADCCommand::ADC_W_CURR, 0x4, 0x0<<ch, ohMask);
  // // get the last data
  // std::vector<uint32_t> last = amc::sca::sendSCACommandWithReply{}(SCAChannel::ADC, SCAADCCommand::ADC_R_DATA, 0x1, 0x0, ohMask);
  // // get the raw data
  // std::vector<uint32_t> raw  = amc::sca::sendSCACommandWithReply{}(SCAChannel::ADC, SCAADCCommand::ADC_R_RAW, 0x1, 0x0, ohMask);
  // // get the offset
  // std::vector<uint32_t> raw  = amc::sca::sendSCACommandWithReply{}(SCAChannel::ADC, SCAADCCommand::ADC_R_OFS, 0x1, 0x0, ohMask);

  if (utils::regExists("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF")) {
    utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  }

  return result;
}

std::vector<uint32_t> amc::sca::readSCAChipID::operator()(uint16_t const& ohMask, bool scaV1) const
{
  if (scaV1)
    return amc::sca::scaCTRLCommand{}(SCACTRLCommand::CTRL_R_ID_V1, ohMask);
  else
    return amc::sca::scaCTRLCommand{}(SCACTRLCommand::CTRL_R_ID_V2, ohMask);

  // ID = DATA[23:0], need to have this set up in the reply function somehow?
}

std::vector<uint32_t> amc::sca::readSCASEUCounter::operator()(uint16_t const& ohMask, bool reset) const
{
  if (reset)
    amc::sca::resetSCASEUCounter{}(ohMask);

  return amc::sca::scaCTRLCommand{}(SCACTRLCommand::CTRL_R_SEU, ohMask);

  // SEU count = DATA[31:0]
}

void amc::sca::resetSCASEUCounter::operator()(uint16_t const& ohMask) const
{
  amc::sca::scaCTRLCommand{}(SCACTRLCommand::CTRL_C_SEU, ohMask);
}

void amc::sca::scaModuleReset::operator()(uint16_t const& ohMask) const
{
  // Does this need to be protected, or do all versions of FW have this register?
  uint32_t origMask = utils::readReg("GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK");
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK", ohMask);

  // Send the reset
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.CTRL.MODULE_RESET", 0x1);

  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK", origMask);
}

void amc::sca::scaHardResetEnable::operator()(bool en) const
{
  utils::writeReg("GEM_AMC.SLOW_CONTROL.SCA.CTRL.TTC_HARD_RESET_EN", uint32_t(en));
}

std::vector<uint32_t> amc::sca::readSCAADCSensor::operator()(SCAADCChannelT const& ch, uint16_t const& ohMask) const
{
  std::vector<uint32_t> result;
  std::vector<uint32_t> outData;

  result = scaADCCommand{}(ch, ohMask);

  int ohIdx = 0;
  for (auto const& val : result) {
      LOG4CPLUS_DEBUG(logger, "Value for OH" << ohIdx << ", SCA-ADC channel 0x" << std::hex << ch << std::dec << " = " << val);
      outData.push_back((utils::bitCheck(ohMask, ohIdx)<<28) | (ohIdx<<24) | (ch<<16) | val);
      ++ohIdx;
  }

  return outData;
}

std::vector<uint32_t> amc::sca::readSCAADCTemperatureSensors::operator()(uint16_t const& ohMask) const
{
  SCAADCChannelT chArr[5] = {
  			      SCAADCChannel::VTTX_CSC_PT100,
  			      SCAADCChannel::VTTX_GEM_PT100,
  			      SCAADCChannel::GBT0_PT100,
  			      SCAADCChannel::V6_FPGA_PT100,
  			      SCAADCChannel::SCA_TEMP
  			    };
  int ohIdx;
  std::vector<uint32_t> result;
  std::vector<uint32_t> outData;
  for (auto const& channelName : chArr) {
    result = scaADCCommand{}(channelName, ohMask);
    ohIdx = 0;
    for (auto const& val : result) {
        LOG4CPLUS_DEBUG(logger, "Temperature for OH" << ohIdx << ", SCA-ADC channel 0x" << std::hex << channelName << " = " << val);
	outData.push_back((utils::bitCheck(ohMask, ohIdx)<<28) | (ohIdx<<24) | (channelName<<16) | val);
	++ohIdx;
    }
  }

  return outData;
}

std::vector<uint32_t> amc::sca::readSCAADCVoltageSensors::operator()(uint16_t const& ohMask) const
{
  SCAADCChannelT chArr[6] = {
  			      SCAADCChannel::PROM_V1P8,
  			      SCAADCChannel::VTTX_VTRX_V2P5,
  			      SCAADCChannel::FPGA_CORE,
  			      SCAADCChannel::SCA_V1P5,
  			      SCAADCChannel::FPGA_MGT_V1P0,
  			      SCAADCChannel::FPGA_MGT_V1P2
  			    };
  int ohIdx;
  std::vector<uint32_t> result;
  std::vector<uint32_t> outData;
  for (auto const& channelName : chArr) {
    result = scaADCCommand{}(channelName, ohMask);
    ohIdx = 0;
    for (auto const& val : result) {
        LOG4CPLUS_DEBUG(logger, "Voltage for OH" << ohIdx << ", SCA-ADC channel 0x" << std::hex << channelName << std::dec << " = " << val);
	outData.push_back((utils::bitCheck(ohMask, ohIdx)<<28) | (ohIdx<<24) | (channelName<<16) | val);
	++ohIdx;
    }
  }

  return outData;
}

std::vector<uint32_t> amc::sca::readSCAADCSignalStrengthSensors::operator()(uint16_t const& ohMask) const
{
  SCAADCChannelT chArr[3] = {
  			      static_cast<SCAADCChannelT>(SCAADCChannel::VTRX_RSSI1),
  			      static_cast<SCAADCChannelT>(SCAADCChannel::VTRX_RSSI2),
  			      static_cast<SCAADCChannelT>(SCAADCChannel::VTRX_RSSI3)
  			    };

  int ohIdx;
  std::vector<uint32_t> result;
  std::vector<uint32_t> outData;
  for (auto const& channelName : chArr) {
    result = scaADCCommand{}(channelName, ohMask);
    ohIdx = 0;
    for (auto const& val : result) {
        LOG4CPLUS_DEBUG(logger, "Signal strength for OH" << ohIdx << ", SCA-ADC channel 0x" << std::hex << channelName << std::dec << " = " << val);
	outData.push_back((utils::bitCheck(ohMask, ohIdx)<<28) | (ohIdx<<24) | (channelName<<16) | val);
	++ohIdx;
    }
  }

  return outData;
}

std::vector<uint32_t> amc::sca::readAllSCAADCSensors::operator()(uint16_t const& ohMask) const
{
  int ohIdx;
  std::vector<uint32_t> result;
  std::vector<uint32_t> outData;
  for (SCAADCChannelT channelName = SCAADCChannel::VTTX_CSC_PT100; channelName<=SCAADCChannel::SCA_TEMP; channelName=SCAADCChannelT(channelName+1)) {
    result = scaADCCommand{}(channelName, ohMask);
    ohIdx = 0;
    for (auto const& val : result) {
      LOG4CPLUS_DEBUG(logger, "Reading of OH" << ohIdx << ", SCA-ADC channel 0x" << std::hex << channelName << std::dec << " = " << val);
      outData.push_back((utils::bitCheck(ohMask, ohIdx)<<28) | (ohIdx<<24) | (channelName<<16) | val);
      ++ohIdx;
    }
  }

  return outData;
}
