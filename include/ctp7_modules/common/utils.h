/*! \file utils.h
 *  \brief util methods for RPC modules running on a Zynq
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "xhal/common/rpc/common.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <ostream>
#include <iomanip>
#include <string>

namespace xhal {
  namespace common {
    namespace rpc {
      template<typename Message>
      inline void serialize(Message &msg, int &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, bool &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, uint8_t &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, uint16_t &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, size_t &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, float &value) {
        msg & *(reinterpret_cast<uint32_t*>(&value));
      }

      /* template<typename Message> */
      /* inline void serialize(Message &msg, double &value) { */
      /*   msg & *(reinterpret_cast<uint32_t*>(&value)); */
      /* } */
    }
  }
}

namespace utils {

    /*!
     *  \brief Contains information stored in the address table for a given register node
     */
    struct RegInfo
    {
        std::string permissions; ///< Named register permissions: r,w,rw
        std::string mode;        ///< Named register mode: s(ingle),b(lock)
        uint32_t    address;     ///< Named register address
        uint32_t    mask;        ///< Named register mask
        uint32_t    size;        ///< Named register size, in 32-bit words

        /*!
         *  \brief With its intrusive serializer
         */
        template<class Message> void serialize(Message & msg) {
            msg & permissions & mode & address & mask & size;
        }

        friend std::ostream& operator<<(std::ostream &o, RegInfo const& r) {
            o << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.address << std::dec << "  "
              << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.mask    << std::dec << "  "
              << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.size    << std::dec << "  "
              << r.mode << "  " << r.permissions;
            return o;
        }
    };

    /*!
     *  \brief Object holding counters of errors encountered during VFAT slow-control transactions
     */
    struct SlowCtrlErrCntVFAT
    {
      uint32_t crc;           ///< GEM_AMC.SLOW_CONTROL.VFAT3.CRC_ERROR_CNT
      uint32_t packet;        ///< GEM_AMC.SLOW_CONTROL.VFAT3.PACKET_ERROR_CNT
      uint32_t bitstuffing;   ///< GEM_AMC.SLOW_CONTROL.VFAT3.BITSTUFFING_ERROR_CNT
      uint32_t timeout;       ///< GEM_AMC.SLOW_CONTROL.VFAT3.TIMEOUT_ERROR_CNT
      uint32_t axi_strobe;    ///< GEM_AMC.SLOW_CONTROL.VFAT3.AXI_STROBE_ERROR_CNT
      uint32_t sum;           ///< Sum of above counters
      uint32_t nTransactions; ///< GEM_AMC.SLOW_CONTROL.VFAT3.TRANSACTION_CNT

      SlowCtrlErrCntVFAT() {
        crc = packet = bitstuffing = timeout = axi_strobe = sum = nTransactions = 0;
      }

      SlowCtrlErrCntVFAT(uint32_t crc, uint32_t packet, uint32_t bitstuffing, uint32_t timeout, uint32_t axi_strobe, uint32_t sum, unsigned nTransactions) :
      crc(crc), packet(packet), bitstuffing(bitstuffing), timeout(timeout), axi_strobe(axi_strobe), sum(sum), nTransactions(nTransactions)
      {}

      SlowCtrlErrCntVFAT operator+(const SlowCtrlErrCntVFAT &vfatErrs) const {
        return SlowCtrlErrCntVFAT(crc+vfatErrs.crc,
                                  packet+vfatErrs.packet,
                                  bitstuffing+vfatErrs.bitstuffing,
                                  timeout+vfatErrs.timeout,
                                  axi_strobe+vfatErrs.axi_strobe,
                                  sum+vfatErrs.sum,
                                  nTransactions+vfatErrs.nTransactions);
      }

      /*!
       *  \brief Function to detect if an overflow occurs during an addition operation
       *         (adapted from https://stackoverflow.com/questions/199333/how-do-i-detect-unsigned-integer-multiply-overflow)
       *
       *  \param \c a number to be added to \c b
       *  \param \c b number to add to \c a
       *
       *  \returns \c 0xffffffff in the case an overflow is detected, otherwise \c a+b
       *
       */
      inline uint32_t overflowTest(const uint32_t &a, const uint32_t &b) {
        const uint32_t overflowTest = a+b;
        if ((overflowTest-b) != a) {
          return 0xffffffff;
        } else {
          return a+b;
        }
      }

      void sumErrors() {
        sum = overflowTest(sum, crc);
        sum = overflowTest(sum, packet);
        sum = overflowTest(sum, bitstuffing);
        sum = overflowTest(sum, timeout);
        sum = overflowTest(sum, axi_strobe);
      }
    };

    /*!
     *  \brief Updates the LMDB object
     *
     *  \param \c at_xml is the name of the XML address table to use to populate the LMDB
     */
    struct update_address_table : public xhal::common::rpc::Method
    {
        void operator()(const std::string &at_xml) const;
    };

    /*!
     *  \brief Read register information from LMDB
     *
     *  \param \c regName Name of node to read from DB
     *
     *  \returns \c RegInfo object with the properties of the found node
     */
    struct readRegFromDB : public xhal::common::rpc::Method
    {
        RegInfo operator()(const std::string &regName) const;
    };
}
#endif
