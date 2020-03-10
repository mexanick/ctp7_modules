#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "ctp7_modules/server/memhub.h"

#include "xhal/common/utils/XHALXMLParser.h"
#include "xhal/extern/lmdb++.h"

#include <libmemsvc.h>

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

namespace utils {

    static constexpr uint32_t LMDB_SIZE = 1UL * 1024UL * 1024UL * 50UL; ///< Maximum size of the LMDB object, currently 50 MiB

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
     *  \param \c regName Register name
     *
     *  \return A \c std::vector<std::string> containing the parsed content of the LMDB register. Is empty if the register does not exist.
     */
    std::vector<std::string> regExists(const std::string &regName);

    /*!
     *  \brief Returns an address of a given register
     *
     *  \param \c regName Register name
     */
    uint32_t getAddress(const std::string &regName);

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
    void writeRawAddress(const uint32_t address, const uint32_t value);

    /*!
     *  \brief Reads a value from raw register address. Register mask is not applied
     *
     *  \param \c address Register address
     */
    uint32_t readRawAddress(const uint32_t address);

    /*!
     *  \brief Writes a value to a raw register. Register mask is not applied
     *
     *  \param \c regName Register name
     *  \param \c value Value to write
     */
    void writeRawReg(const std::string &regName, const uint32_t value);

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
     *  \brief Reads a value from register. Register mask is applied. An \c std::runtime_error is thrown if the register cannot be read.
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
    SlowCtrlErrCntVFAT repeatedRegRead(const std::string & regName, const bool breakOnFailure=true, const uint32_t nReads=1000);

    /*!
     *  \brief Writes a value to a register. Register mask is applied. An \c std::runtime_error is thrown if the register cannot be read.
     *
     *  \param \c regName Register name
     *  \param \c value Value to write
     */
    void writeReg(const std::string &regName, const uint32_t value);

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

}

#endif
