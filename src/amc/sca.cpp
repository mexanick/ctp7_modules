/*!
 * \file amc/sca.cpp
 * \brief AMC SCA methods for RPC modules
 */

#include "amc/sca.h"
#include "hw_constants.h"

uint32_t formatSCAData(uint32_t const& data)
{
  return (
          ((data&0xff000000)>>24) +
          ((data>>8)&0x0000ff00)  +
          ((data&0x0000ff00)<<8) +
          ((data&0x000000ff)<<24)
          );
}

void sendSCACommand(localArgs* la, uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask)
{
  // FIXME: DECIDE WHETHER TO HAVE HERE // writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",         0xffffffff);
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.LINK_ENABLE_MASK",       ohMask);
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_CHANNEL",ch);
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_COMMAND",cmd);
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_LENGTH", len);
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_DATA",   formatSCAData(data));
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_CMD.SCA_CMD_EXECUTE",0x1);
}

std::vector<uint32_t> sendSCACommandWithReply(localArgs* la, uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask)
{
  // FIXME: DECIDE WHETHER TO HAVE HERE // uint32_t monMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  // FIXME: DECIDE WHETHER TO HAVE HERE // writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);

  sendSCACommand(la, ch, cmd, len, data, ohMask);

  // read reply from OptoHybrids attached to an AMC
  std::vector<uint32_t> reply;
  reply.reserve(amc::OH_PER_AMC);
  for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
    if ((ohMask >> oh) & 0x1) {
      std::stringstream regName;
      regName << "GEM_AMC.SLOW_CONTROL.SCA.MANUAL_CONTROL.SCA_REPLY_OH"
              << oh << ".SCA_RPY_DATA";
      reply.push_back(formatSCAData(readReg(la,regName.str())));
    } else {
      // FIXME Sensible null value?
      reply.push_back(0);
    }
  }
  // FIXME: DECIDE WHETHER TO HAVE HERE // writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  return reply;
}

std::vector<uint32_t> scaCTRLCommand(localArgs* la, SCACTRLCommandT const& cmd, uint16_t const& ohMask, uint8_t const& len, uint32_t const& data)
{
  uint32_t monMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);

  std::vector<uint32_t> result;
  switch (cmd) {
  case SCACTRLCommand::CTRL_R_ID_V2:
    result = sendSCACommandWithReply(la, 0x14, cmd, 0x1, 0x1, ohMask);
  case SCACTRLCommand::CTRL_R_ID_V1:
    result = sendSCACommandWithReply(la, 0x14, cmd, 0x1, 0x1, ohMask);
  case SCACTRLCommand::CTRL_R_SEU:
    result = sendSCACommandWithReply(la, 0x13, cmd, 0x1, 0x0, ohMask);
  case SCACTRLCommand::CTRL_C_SEU:
    result = sendSCACommandWithReply(la, 0x13, cmd, 0x1, 0x0, ohMask);
  case SCACTRLCommand::CTRL_W_CRB:
    sendSCACommand(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_W_CRC:
    sendSCACommand(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_W_CRD:
    sendSCACommand(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRB:
    result = sendSCACommandWithReply(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRC:
    result = sendSCACommandWithReply(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  case SCACTRLCommand::CTRL_R_CRD:
    result = sendSCACommandWithReply(la, SCAChannel::CTRL, cmd, len, data, ohMask);
  default:
    // maybe don't do this by default, return error or invalid option?
    result = sendSCACommandWithReply(la, SCAChannel::CTRL, SCACTRLCommand::GET_DATA, len, data, ohMask);
  }

  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  return result;
}

std::vector<uint32_t> scaI2CCommand(localArgs* la, SCAI2CChannelT const& ch, SCAI2CCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask)
{

  // enable the I2C bus through the CTRL CR{B,C,D} registers
  // 16 I2C channels available
  // 4 I2C modes of communication
  // I2C frequency selection
  // Allows RMW transactions

  std::vector<uint32_t> result;

  uint32_t monMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);

  sendSCACommand(la, ch, cmd, len, data, ohMask);

  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);

  return result;
}

std::vector<uint32_t> scaGPIOCommandLocal(localArgs* la, SCAGPIOCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask)
{
  // enable the GPIO bus through the CTRL CRB register, bit 2
  uint32_t monMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);

  std::vector<uint32_t> reply = sendSCACommandWithReply(la, SCAChannel::GPIO, cmd, len, data, ohMask);

  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);
  return reply;
}

std::vector<uint32_t> scaADCCommand(localArgs* la, SCAADCChannelT const& ch, SCAADCCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask)
{
  uint32_t monMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF");
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF",       0xffffffff);

  // enable the ADC bus through the CTRL CRD register, bit 4
  // select the ADC channel
  sendSCACommand(la, SCAChannel::ADC, SCAADCCommand::ADC_W_MUX, 0x4, ch, ohMask);

  // enable/disable the current sink
  sendSCACommand(la, SCAChannel::ADC, SCAADCCommand::ADC_W_CURR, 0x4, 0x1<<ch, ohMask);
  // start the conversion and get the result
  std::vector<uint32_t> result = sendSCACommandWithReply(la, SCAChannel::ADC, SCAADCCommand::ADC_GO, 0x4, 0x1, ohMask);

  // // get the last data
  // std::vector<uint32_t> last = sendSCACommandWithReply(la, SCAChannel::ADC, SCAADCCommand::ADC_R_DATA, 0x1, 0x0, ohMask);
  // // get the raw data
  // std::vector<uint32_t> raw  = sendSCACommandWithReply(la, SCAChannel::ADC, SCAADCCommand::ADC_R_RAW, 0x1, 0x0, ohMask);
  // // get the offset
  // std::vector<uint32_t> raw  = sendSCACommandWithReply(la, SCAChannel::ADC, SCAADCCommand::ADC_R_OFS, 0x1, 0x0, ohMask);

  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.MONITORING_OFF", monMask);

  return result;
}

std::vector<uint32_t> readSCAChipIDLocal(localArgs* la, uint16_t const& ohMask, bool scaV1)
{
  if (scaV1)
    return scaCTRLCommand(la, SCACTRLCommand::CTRL_R_ID_V1, ohMask);
  else
    return scaCTRLCommand(la, SCACTRLCommand::CTRL_R_ID_V2, ohMask);

  // ID = DATA[23:0], need to have this set up in the reply function somehow?
}

std::vector<uint32_t> readSCASEUCounterLocal(localArgs* la, uint16_t const& ohMask, bool reset)
{
  if (reset)
    resetSCASEUCounterLocal(la, ohMask);

  return scaCTRLCommand(la, SCACTRLCommand::CTRL_R_SEU, ohMask);

  // SEU count = DATA[31:0]
}

void resetSCASEUCounterLocal(localArgs* la, uint16_t const& ohMask)
{
  scaCTRLCommand(la, SCACTRLCommand::CTRL_C_SEU, ohMask);
}

void scaModuleResetLocal(localArgs* la, uint16_t const& ohMask)
{
  // Does this need to be protected, or do all versions of FW have this register?
  uint32_t origMask = readReg(la,"GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK");
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK", ohMask);

  // Send the reset
  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.CTRL.MODULE_RESET", 0x1);

  writeReg(la,"GEM_AMC.SLOW_CONTROL.SCA.CTRL.SCA_RESET_ENABLE_MASK", origMask);
}

void scaHardResetEnableLocal(localArgs* la, bool en)
{
  writeReg(la, "GEM_AMC.SLOW_CONTROL.SCA.CTRL.TTC_HARD_RESET_EN", uint32_t(en));
}

/** RPC callbacks */
void scaModuleReset(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t ohMask = request->get_word("ohMask");

  scaModuleResetLocal(&la, ohMask);

  rtxn.abort();
}

void readSCAChipID(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t ohMask = request->get_word("ohMask");
  bool     scaV1  = request->get_word("scaV1");
  std::vector<uint32_t> scaChipIDs = readSCAChipIDLocal(&la, ohMask, scaV1);

  rtxn.abort();
}

void readSCASEUCounter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t ohMask = request->get_word("ohMask");
  bool     reset  = request->get_word("reset");
  std::vector<uint32_t> seuCounts = readSCASEUCounterLocal(&la, ohMask, reset);

  rtxn.abort();
}

void resetSCASEUCounter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t ohMask = request->get_word("ohMask");
  resetSCASEUCounterLocal(&la, ohMask);

  rtxn.abort();
}
