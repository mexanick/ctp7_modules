/*!
 * \file amc/daq.cpp
 * \brief AMC DAQ methods for RPC modules
 */

#include "amc/daq.h"
#include "amc/ttc.h"
#include "amc/sca.h"

void enableDAQLinkLocal(localArgs* la, uint32_t const& enableMask)
{
  LOG4CPLUS_DEBUG(logger, "enableDAQLinkLocal called");
  // writeReg(la, "GEM_AMC.DAQ.CONTROL.INPUT_ENABLE_MASK", enableMask);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAQ_ENABLE", 0x1);
  return;
}

void disableDAQLinkLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.INPUT_ENABLE_MASK", 0x0);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAQ_ENABLE",        0x0);
}

void setZSLocal(localArgs* la, bool en)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.ZERO_SUPPRESSION_EN", uint32_t(en));
}

void disableZSLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.ZERO_SUPPRESSION_EN", 0x0);
}

void resetDAQLinkLocal(localArgs* la, uint32_t const& davTO, uint32_t const& ttsOverride)
{
  LOG4CPLUS_DEBUG(logger, "resetDAQLinkLocal called");
  writeReg(la, "GEM_AMC.DAQ.CONTROL.RESET", 0x1);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.RESET", 0x0);
  // disableDAQLinkLocal(la);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAV_TIMEOUT", davTO);
  setDAQLinkInputTimeoutLocal(la);
  // setDAQLinkInputTimeoutLocal(la,davTO);
  // writeReg(la, "GEM_AMC.DAQ.TTS_OVERRIDE", ttsOverride);
  return;
}

uint32_t getDAQLinkControlLocal(localArgs* la)
{
  LOG4CPLUS_WARN(logger, "getDAQLinkControl not implemented");
  return 0x0;
}

uint32_t getDAQLinkStatusLocal(localArgs* la)
{
  LOG4CPLUS_WARN(logger, "getDAQLinkStatus not implemented");
  return 0x0;
}

bool daqLinkReadyLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.DAQ_LINK_RDY"));
}

bool daqClockLockedLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.DAQ_CLK_LOCKED"));
}

bool daqTTCReadyLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.TTC_RDY"));
}

uint8_t daqTTSStateLocal(localArgs* la)
{
  return uint8_t(readReg(la, "GEM_AMC.DAQ.STATUS.TTS_STATE"));
}

bool daqAlmostFullLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.DAQ_AFULL"));
}

bool l1aFIFOIsEmptyLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.L1A_FIFO_IS_EMPTY"));
}

bool l1aFIFOIsAlmostFullLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.L1A_FIFO_IS_NEAR_FULL"));
}

bool l1aFIFOIsFullLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.L1A_FIFO_IS_FULL"));
}

bool l1aFIFOIsUnderflowLocal(localArgs* la)
{
  return bool(readReg(la, "GEM_AMC.DAQ.STATUS.L1A_FIFO_IS_UNDERFLOW"));
}

uint32_t getDAQLinkEventsSentLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_STATUS.EVT_SENT");
}

uint32_t getDAQLinkL1AIDLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_STATUS.L1AID");
}

uint32_t getDAQLinkL1ARateLocal(localArgs* la)
{
  LOG4CPLUS_WARN(logger, "getDAQLinkL1ARate not implemented");
  return 0x0;
}

uint32_t getDAQLinkDisperErrorsLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_STATUS.DISPER_ERR");
}

uint32_t getDAQLinkNonidentifiableErrorsLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_STATUS.NOTINTABLE_ERR");
}

uint32_t getDAQLinkInputMaskLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.CONTROL.INPUT_ENABLE_MASK");
}

uint32_t getDAQLinkDAVTimeoutLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.CONTROL.DAV_TIMEOUT");
}

uint32_t getDAQLinkDAVTimerLocal(localArgs* la, bool const& max)
{
  uint32_t maxtimer  = readReg(la, "GEM_AMC.DAQ.EXT_STATUS.MAX_DAV_TIMER");
  uint32_t lasttimer = readReg(la, "GEM_AMC.DAQ.EXT_STATUS.LAST_DAV_TIMER");
  la->response->set_word("max", maxtimer);
  la->response->set_word("last",lasttimer);

  if (max)
    return maxtimer;
  else
    return lasttimer;
}

uint32_t getLinkDAQStatusLocal(localArgs* la, uint8_t const& gtx)
{
  LOG4CPLUS_WARN(logger, "getLinkDAQStatus not implemented");
  return 0x0;
}

uint32_t getLinkDAQCountersLocal(localArgs* la, uint8_t const& gtx, uint8_t const& mode)
{
  LOG4CPLUS_WARN(logger, "getLinkDAQCounters not implemented");
  return 0x0;
}

uint32_t getLinkLastDAQBlockLocal(localArgs* la, uint8_t const& gtx)
{
  LOG4CPLUS_WARN(logger, "getLinkLastDAQBlock not implemented");
  return 0x0;
}

uint32_t getDAQLinkInputTimeoutLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_CONTROL.INPUT_TIMEOUT");
}

uint32_t getDAQLinkRunTypeLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_TYPE");
}

uint32_t getDAQLinkRunParametersLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAMS");
}

uint32_t getDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parameter)
{
  std::stringstream regname;
  regname << "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAM" << int(parameter);
  return readReg(la, regname.str());
}


void setDAQLinkInputTimeoutLocal(localArgs* la, uint32_t const& inputTO)
{
  LOG4CPLUS_WARN(logger, "setDAQLinkInputTimeout not implemented");
}

void setDAQLinkRunTypeLocal(localArgs* la, uint32_t const& rtype)
{
  writeReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_TYPE", rtype);
}

void setDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parN, uint8_t const& rparam)
{
  if (parN < 1 || parN > 3) {
    std::stringstream errmsg;
    errmsg << "Attempting to set DAQ link run parameter " << parN << ": outside expectation (1-3)";
    la->response->set_string("error", errmsg.str());
    return;
  }
  std::stringstream regBase;
  regBase << "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAM" << (int)parN;
  writeReg(la, regBase.str(), rparam);
}

void setDAQLinkRunParametersLocal(localArgs* la, uint32_t const& rparams)
{
  writeReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAMS", rparams);
}


/** RPC callbacks */
void enableDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t enableMask = request->get_word("enableMask");
  enableDAQLinkLocal(&la, enableMask);
  rtxn.abort();
}

void disableDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  disableDAQLinkLocal(&la);
  rtxn.abort();
}

void setZS(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool enable = request->get_word("enable");
  setZSLocal(&la,enable);
  rtxn.abort();
}

void disableZS(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  disableZSLocal(&la);
  rtxn.abort();
}

void resetDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  // what if these are not provided, want to use the function defaults, or should these be defined upstream?
  uint32_t davTO       = request->get_word("davTO");
  uint32_t ttsOverride = request->get_word("ttsOverride");
  resetDAQLinkLocal(&la, davTO, ttsOverride);
  rtxn.abort();
}

void getDAQLinkControl(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkControlLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkStatus(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkStatusLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void daqLinkReady(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqLinkReadyLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void daqClockLocked(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqClockLockedLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void daqTTCReady(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqTTCReadyLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void daqTTSState(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = daqTTSStateLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void daqAlmostFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = daqAlmostFullLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void l1aFIFOIsEmpty(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsEmptyLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void l1aFIFOIsAlmostFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsAlmostFullLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void l1aFIFOIsFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsFullLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void l1aFIFOIsUnderflow(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsUnderflowLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkEventsSent(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkEventsSentLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkL1AID(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkL1AIDLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkL1ARate(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkL1ARateLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkDisperErrors(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkDisperErrorsLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkNonidentifiableErrors(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkNonidentifiableErrorsLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkInputMask(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkInputMaskLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}

void getDAQLinkDAVTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkDAVTimeoutLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}
void getDAQLinkDAVTimer(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool max = request->get_word("max");
  uint32_t res = getDAQLinkDAVTimerLocal(&la, max);
  response->set_word("result", res);
  rtxn.abort();
}
void getLinkDAQStatus(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx = request->get_word("gtx");
  uint32_t res = getLinkDAQStatusLocal(&la, gtx);
  response->set_word("result", res);
  rtxn.abort();
}
void getLinkDAQCounters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx  = request->get_word("gtx");
  uint8_t mode = request->get_word("mode");
  uint32_t res = getLinkDAQCountersLocal(&la, gtx, mode);
  response->set_word("result", res);
  rtxn.abort();
}
void getLinkLastDAQBlock(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx  = request->get_word("gtx");
  uint32_t res = getLinkLastDAQBlockLocal(&la, gtx);
  response->set_word("result", res);
  rtxn.abort();
}
void getDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkInputTimeoutLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}
void getDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkRunTypeLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}
void getDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkRunParametersLocal(&la);
  response->set_word("result", res);
  rtxn.abort();
}
void getDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t parameter  = request->get_word("parameter");
  uint32_t res = getDAQLinkRunParameterLocal(&la, parameter);
  response->set_word("result", res);
  rtxn.abort();
}
void setDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t inputTO = request->get_word("inputTO");
  setDAQLinkInputTimeoutLocal(&la, inputTO);
  rtxn.abort();
}
void setDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t runType = request->get_word("runType");
  setDAQLinkRunTypeLocal(&la, runType);
  rtxn.abort();
}
void setDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t parN   = request->get_word("parameterN");
  uint8_t runPar = request->get_word("runParameter");
  setDAQLinkRunParameterLocal(&la, parN, runPar);
  rtxn.abort();
}
void setDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t runPars    = request->get_word("runParameters");
  setDAQLinkRunParametersLocal(&la, runPars);
  rtxn.abort();
}
/** Composite RPC methods */
void configureDAQModule(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  bool enableZS     = request->get_word("enableZS");
  bool doPhaseShift = request->get_word("doPhaseShift");
  uint32_t runType  = request->get_word("runType");

  // FIXME: Should the full routine be run in the event of an error?
  // Curently none of these calls will throw/error directly, could handle that here?
  scaHardResetEnableLocal(&la, false);
  ttcCounterResetLocal(&la);

  // FIXME: if we include this as part of the sequence, needs to go in amc.cpp (links with ttc.cpp)
  if (doPhaseShift) {
    bool relock        = request->get_word("relock");
    bool bc0LockPSMode = request->get_word("bc0LockPSMode");
    ttcMMCMPhaseShiftLocal(&la, relock, bc0LockPSMode);
  }

  setL1AEnableLocal(&la, false);
  disableDAQLinkLocal(&la);
  resetDAQLinkLocal(&la);
  enableDAQLinkLocal(&la, 0x4); // FIXME
  setZSLocal(&la, enableZS);
  setDAQLinkRunTypeLocal(&la, 0x0);
  setDAQLinkRunParametersLocal(&la, 0xfaac);

  rtxn.abort();
}

void enableDAQModule(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  bool enableZS     = request->get_word("enableZS");

  // FIXME: Should the full routine be run in the event of an error?
  // Curently none of these calls will throw/error directly, could handle that here?
  ttcModuleResetLocal(&la);
  enableDAQLinkLocal(&la, 0x4); // FIXME
  resetDAQLinkLocal(&la);
  setZSLocal(&la, enableZS);
  setL1AEnableLocal(&la, true);

  rtxn.abort();
}
