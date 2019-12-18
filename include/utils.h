/*! \file utils.h
 *  \brief util methods for RPC modules running on a Zynq
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef UTILS_H
#define UTILS_H

#include "moduleapi.h"
#include "memhub.h"
#include <libmemsvc.h>

#include "lmdb_cpp_wrapper.h"

#include "xhal/common/utils/XHALXMLParser.h"
#include "xhal/common/rpc/common.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

extern memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations
/* extern log4cplus logger; /// \var global logger */

/*!
 *  \brief Environment variable name storing the \c log4cplus configuration filename
 */
constexpr auto LOGGING_CONFIGURATION_ENV = "RPCSVC_LOGGING_CONF";

/*!
 *  \brief Default \c log4cplus configuration used when the configuration file cannot be read
 */
constexpr auto LOGGING_DEFAULT_CONFIGURATION = R"----(
log4cplus.rootLogger=INFO,syslog
log4cplus.appender.syslog=log4cplus::SysLogAppender
log4cplus.appender.syslog.ident=rpcsvc
log4cplus.appender.syslog.facility=user
log4cplus.appender.syslog.layout=log4cplus::PatternLayout
log4cplus.appender.syslog.layout.ConversionPattern= %h[%i] - %M - %m
)----";

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
     *  \brief Contains arguments required to execute an LMDB transaction
     */
    struct LocalArgs
    {
        lmdb::txn &rtxn; ///< LMDB transaction handle
        lmdb::dbi &dbi;  ///< LMDB individual database handle
    };

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

    static constexpr uint32_t LMDB_SIZE = 1UL * 1024UL * 1024UL * 50UL; ///< Maximum size of the LMDB object, currently 50 MiB

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
     *  \brief Tokenize a string based on a delimieter
     *
     *  \param \c s The \c std::string object to tokenize
     *  \param \c delim \c char value to use as tokenizer
     *  \tparam \c Out \c Out value to use as tokenizer
     */
    template<typename Out>
    void split(const std::string &s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    /*!
     *  \brief Tokenize a string based on a delimieter
     *
     *  \param \c s The \c std::string object to tokenize
     *  \param \c delim \c char value to use as tokenizer
     */
    std::vector<std::string> split(const std::string &s, char delim);

    /*!
     *  \brief Serialize an \c xhal::common::utils::Node
     *
     *  \param \c n The \c xhal::common::utils::Node object to serialize
     */
    std::string serialize(xhal::common::utils::Node n);

    /*!
     * \brief This function initializes the \c log4cplus logging system
     *
     * It first tries to read the \c LOGGING_CONFIGURATION_FILENAME.
     * If the file is not found, it defaults to the embedded configuration LOGGING_DEFAULT_CONFIGURATION.
     */
    void initLogging();

    /*!
     *  \brief This macro is used to terminate a function if an error occurs.
     *         It logs the message  and returns the \c error_code value.
     *
     *  \param \c message The \c std::string error message.
     *  \param \c error_code Value that is passed to the \c return statement.
     */
#define EMIT_RPC_ERROR(message, error_code) {           \
      LOG4CPLUS_ERROR(logger, message);                 \
      return error_code;                                \
    }

    /*!
     *  \brief return 1 if the given bit in word is 1 else 0
     *
     *  \param \c word: an unsigned int of 32 bit
     *  \param \c bit: integer that specifies a particular bit position
     */
    uint32_t bitCheck(uint32_t word, int bit);

    /*!
     *  \brief returns the number of nonzero bits in an integer
     *
     *  \param \c value integer to check the number of nonzero bits
     */
    uint32_t getNumNonzeroBits(uint32_t value);

    /*!
     *  \brief returns the number of nonzero bits in an integer
     *
     *  \param \c value integer to check the number of nonzero bits
     */
    uint32_t getNumNonzeroBits(uint32_t value);

    /*!
     *  \brief Returns whether or not a named register can be found in the LMDB
     *
     *  \param \c la Local arguments structure
     *  \param \c regName Register name
     *  \param \c db_res pointer to a lmdb::val result that the calling function requires
     */
    bool regExists(const std::string & regName, lmdb::val *db_res=nullptr);

    /*!
     *  \brief Returns the mask for a given register
     *
     *  \param \c regName Register name
     */
    uint32_t getMask(const std::string &regName);

    /*!
     *  \brief Writes a value to a raw register address. Register mask is not applied
     *
     *  \param \c address Register address
     *  \param \c value Value to write
     */
    void writeRawAddress(uint32_t address, uint32_t value);

    /*!
     *  \brief Reads a value from raw register address. Register mask is not applied
     *
     *  \param \c address Register address
     */
    uint32_t readRawAddress(uint32_t address);

    /*!
     *  \brief Returns an address of a given register
     *
     *  \param \c regName Register name
     */
    uint32_t getAddress(const std::string &regName);

    /*!
     *  \brief Writes given value to the address. Register mask is not applied
     *
     *  \param \c db_res LMDB call result
     *  \param \c value Value to write
     */
    void writeAddress(lmdb::val &db_res, uint32_t value);

    /*!
     *  \brief Reads given value to the address. Register mask is not applied
     *
     *  \param \c db_res LMDB call result
     */
    uint32_t readAddress(lmdb::val &db_res);

    /*!
     *  \brief Writes a value to a raw register. Register mask is not applied
     *
     *  \param \c regName Register name
     *  \param \c value Value to write
     */
    void writeRawReg(const std::string &regName, uint32_t value);

    /*!
     *  \brief Reads a value from raw register. Register mask is not applied
     *
     *  \param \c regName Register name
     */
    uint32_t readRawReg(const std::string &regName);

    /*!
     *  \brief Returns the data with register mask applied
     *
     *  \param \c data Register data
     *  \param \c mask Register mask
     */
    uint32_t applyMask(uint32_t data, uint32_t mask);

    /*!
     *  \brief Reads a value from register. Register mask is applied. Will return 0xdeaddead if register is no accessible
     *
     *  \param \c regName Register name
     */
    uint32_t readReg(const std::string &regName);

    /*!
     *  \brief Reads a block of values from a contiguous address space.
     *
     *  \param \c regName Register name of the block to be read
     *  \param \c size number of words to read (should this just come from the register properties?
     *  \param \c result Pointer to an array to hold the result
     *  \param \c offset Start reading from an offset from the base address returned by regName
     *
     *  \returns the number of uint32_t words in the result (or better to return a std::vector?
     */
    uint32_t readBlock(const std::string &regName, uint32_t *result, const uint32_t &size, const uint32_t &offset=0);
    /* std::vector<uint32_t> readBlock(const std::string &regName, const uint32_t &size, const uint32_t &offset=0); */

    /*!
     *  \brief Reads a block of values from a contiguous address space.
     *
     *  \param \c regAddr Register address of the block to be read
     *  \param \c size number of words to read
     *  \param \c result Pointer to an array to hold the result
     *  \param \c offset Start reading from an offset from the base address regAddr
     *
     *  \returns the number of uint32_t words in the result (or better to return a std::vector?
     */
    uint32_t readBlock(const uint32_t &regAddr,  uint32_t *result, const uint32_t &size, const uint32_t &offset=0);
    /* std::vector<uint32_t> readBlock(const uint32_t &regAddr, const uint32_t &size, const uint32_t &offset=0); */

    /*!
     *  \brief Reads a register for nReads and then counts the number of slow control errors observed.
     *
     *  \param \c regName Register name
     *  \param \c breakOnFailure stop attempting to read regName before nReads is reached if a failed read occurs
     *  \param \c nReads number of times to attempt to read regName
     */
    SlowCtrlErrCntVFAT repeatedRegRead(const std::string & regName, bool breakOnFailure=true, uint32_t nReads=1000);

    /*!
     *  \brief Writes a value to a register. Register mask is applied
     *
     *  \param \c regName Register name
     *  \param \c value Value to write
     */
    void writeReg(const std::string &regName, uint32_t value);

    /*!
     *  \brief Writes a block of values to a contiguous address space.
     *
     *  \detail Block writes are allowed on 'single' registers, provided:
     *           * The size of the data to be written is 1
     *           * The register does not have a mask
     *          Block writes are allowed on 'block' and 'fifo/incremental/port' type addresses provided:
     *          * The size does not overrun the block as defined in the address table
     *          * Including cases where an offset is provided, baseaddr+offset+size > blocksize
     *
     *  \param \c regName Register name of the block to be written
     *  \param \c values Values to write to the block
     *  \param \c offset Start writing at an offset from the base address returned by regName
     */
    void writeBlock(const std::string &regName, const uint32_t *values, const uint32_t &size, const uint32_t &offset=0);

    /*!
     *  \brief Writes a block of values to a contiguous address space.
     *
     *  \detail Block writes are allowed on 'single' registers, provided:
     *           * The size of the data to be written is 1
     *           * The register does not have a mask
     *          Block writes are allowed on 'block' and 'fifo/incremental/port' type addresses provided:
     *           * The size does not overrun the block as defined in the address table
     *           * Including cases where an offset is provided, baseaddr+offset+size > blocksize
     *
     *  \param \c regAddr Register address of the block to be written
     *  \param \c values Values to write to the block
     *  \param \c size number of words to write
     *  \param \c offset Start writing at an offset from the base address regAddr
     */
    void writeBlock(const uint32_t &regAddr, const uint32_t *values, const uint32_t &size, const uint32_t &offset=0);

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
