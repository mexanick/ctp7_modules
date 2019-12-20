/*! \file optohybrid.h
 *  \brief RPC module for \c OptoHybrid methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Brin Dorney <brian.l.dorney@cern.ch>
 */
#ifndef OPTOHYBRID_H
#define OPTOHYBRID_H

#include "ctp7_modules/common/utils.h"
#include "ctp7_modules/common/vfat_parameters.h"

#include <map>
#include <string>
#include <vector>

namespace oh {

  /*!
   *  \brief Performs a write transaction on a specified regiser for unmasked the VFATs of a specified \c OptoHybrid
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c regName Register name
   *  \param \c value Register value to write
   *  \param \c mask VFAT mask. Default: no chips will be masked
   */
  struct broadcastWrite : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const std::string &regName, const uint32_t &value, const uint32_t &mask=0xff000000) const;
  };

  /*!
   *  \brief Performs a read transaction on a specified regiser on unmasked VFATs of a specified \c OptoHybrid
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c regName Register name
   *  \param \c mask VFAT mask. Default: no chips will be masked
   *
   *  \returns std::vector<uint32_t> holding the results of the broadcast read
   */
  struct broadcastRead : public xhal::common::rpc::Method
  {
    std::vector<uint32_t> operator()(const uint32_t &ohN, const std::string &regName, const uint32_t &mask=0xff000000) const;
  };

  /*!
   *  \brief Sets default values to VFAT parameters. VFATs will remain in sleep mode
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c mask VFAT mask. Default: no chips will be masked
   */
  struct biasAllVFATs : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const uint32_t &mask=0xff000000) const;
  };

  /*!
   *  \briefSets VFATs to run mode
   *
   *  \param \c ohN \c OptoHybrid optical link number (string)
   *  \param \c mask VFAT mask. Default: no chips will be masked
   */
  struct setAllVFATsToRunMode : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const uint32_t &mask=0xff000000) const;
  };

  /*!
   *  \brief Sets VFATs to sleep mode
   *
   *  \param \c ohN \c OptoHybrid optical link number (string)
   *  \param \c mask VFAT mask. Default: no chips will be masked
   */
  struct setAllVFATsToSleepMode : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const uint32_t &mask=0xff000000) const;
  };

  /*!
   *  \brief Sets trimming DAC parameters for each channel of each chip
   *
   *  \throws \c std::runtime_error if the \c configFile is not able to be opened
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c configFile Configuration file with trimming parameters
   */
  struct loadTRIMDAC : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const std::string &configFile) const;
  };

  /*!
   *  \brief Returns a list of the most important monitoring registers of \c OptoHybrids
   *
   *  \param \c ohMask Bit mask of enabled optical links
   *
   *  \returns std::map with keys corresponding to the OH status registers
               and values are stored in an std::vector with one entry for each OH
   */
  struct statusOH : public xhal::common::rpc::Method
  {
    std::map<std::string, std::vector<uint32_t>> operator()(const uint32_t &ohMask) const;
  };

  /*!
   *  \brief Disables calibration pulse in channels between chMin and chMax
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c mask VFAT mask. Default: no chips will be masked
   *  \param \c chMin Minimal channel number
   *  \param \c chMax Maximal channel number
   */
  struct stopCalPulse2AllChannels : public xhal::common::rpc::Method
  {
    void operator()(const uint32_t &ohN, const uint32_t &mask, const uint32_t &ch_min, const uint32_t &ch_max) const;
  };

  /*!
   *  \brief Sets threshold and trim range for each VFAT2 chip
   *
   *  \deprecated
   *
   *  \throws \c std::runtime_error if \c configFile cannot be opened
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c config_file Configuration file with VT1 and trim values. Optional (could be supplied as an empty string)
   *  \param \c vt1. Default: 0x64, used if the config_file is not provided
   */
  struct [[deprecated]] loadVT1 : public xhal::common::rpc::Method
  {
    // __attribute__((__deprecated__))
    void operator()(const uint32_t &ohN, const std::string &config_file="", const uint32_t &vt1=0x64) const;
  };

  /*!
   *  \brief Configures VFAT chips (V2B only). Calls biasVFATs, loads VT1 and TRIMDAC values from the config files
   *
   *  \deprecated
   *
   *  \param \c ohN
   *  \param \c trimConfigFile
   *  \param \c threshConfigFile
   *  \param \c vt1
   *  \param \c setRunMode
   */
  struct [[deprecated]] configureVFATs : public xhal::common::rpc::Method
  {
    // __attribute__((__deprecated__))
      void operator()(const uint16_t &ohN, const std::string &trimConfigFile, const std::string &threshConfigFile, const uint8_t &vt1, const bool &setRunMode=false) const;
  };

  /*!
   *  \brief Configures V2b FW scan module
   *
   *     Configure the firmware scan controller
   *      mode: 0 Threshold scan
   *            1 Threshold scan per channel
   *            2 Latency scan
   *            3 s-curve scan
   *            4 Threshold scan with tracking data
   *      vfat: for single VFAT scan, specify the VFAT number
   *            for ULTRA scan, specify the VFAT mask
   *
   *  \deprecated
   *
   *  \throws \c std::runtime_error if a scan is already running, as determined by the \c MONITOR.STATUS register
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c vfatN VFAT chip position
   *  \param \c scammode Scan mode
   *  \param \c useUltra Set to 1 in order to use the ultra scan
   *  \param \c mask VFAT chips mask
   *  \param \c ch Channel to scan
   *  \param \c nevts Number of events per scan point
   *  \param \c dacMin Minimal value of scan variable
   *  \param \c dacMax Maximal value of scan variable
   *  \param \c dacStep Scan variable change step
   */
  struct [[deprecated]] configureScanModule : public xhal::common::rpc::Method
  {
    // __attribute__((__deprecated__))
    void operator()(const uint32_t &ohN, const uint32_t &vfatN, const uint32_t &scanmode, const bool &useUltra, const uint32_t &mask, const uint32_t &ch, const uint32_t &nevts, const uint32_t &dacMin, const uint32_t &dacMax, const uint32_t &dacStep) const;
  };

  /*!
   *  \brief Returns results of an ultra scan routine
   *
   *  \deprecated
   *
   *  \param \c outData Pointer to output data array
   *  \param \c nevts Number of events per scan point
   *  \param \c dacMin Minimal value of scan variable
   *  \param \c dacMax Maximal value of scan variable
   *  \param \c dacStep Scan variable change step
   */
  struct [[deprecated]] getUltraScanResults : public xhal::common::rpc::Method
  {
    /* __attribute__((__deprecated__)) */
    std::vector<uint32_t> operator()(const uint32_t &ohN, const uint32_t &nevts, const uint32_t &dacMin, const uint32_t &dacMax, const uint32_t &dacStep) const;
  };

  /*!
   *  \brief Prints V2b FW scan module configuration
   *
   *  \deprecated
   *
   *  \throws \c std::runtime_error if reading one of the registers fails
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c useUltra Set to 1 in order to use the ultra scan
   */
  struct [[deprecated]] printScanConfiguration : public xhal::common::rpc::Method
  {
    // __attribute__((__deprecated__))
    void operator()(const uint32_t &ohN, const bool &useUltra) const;
  };

  /*!
   *  \brief Starts V2b FW scan module
   *
   *  \deprecated
   *
   *  \throws \c std::runtime_error if:
   *          A scan is already running, as reported by \c MONITOR.STATUS
   *          An error is detected, as reported by \c MONITOR.ERROR
   *
   *  \param \c ohN \c OptoHybrid optical link number
   *  \param \c useUltra Set to 1 in order to use the ultra scan
   */
  struct [[deprecated]] startScanModule : public xhal::common::rpc::Method
  {
    // __attribute__((__deprecated__))
    void operator()(const uint32_t &ohN, const bool &useUltra) const;
  };
}

#endif
