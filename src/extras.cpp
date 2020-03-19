/*! \file extras.cpp
 *  \brief Extra register operations methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 */

#include "ctp7_modules/server/memhub.h"
#include "ctp7_modules/common/utils.h"
#include "ctp7_modules/server/utils.h"

#include <libmemsvc.h>

#include "xhal/extern/ModuleManager.h"

#include <vector>
#include <unordered_map>

memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations

namespace extras {
  /*!
   *  \brief Sequentially reads a block of values from a contiguous address space.
   *         Register mask is not applied
   *
   *  \throws \c std::runtime_error if the \c memhub_read has an nonzero exit status
   *
   *  \param \c addr address of the block to read
   *  \param \c count how many 32-bit words to read out of the block
   *
   *  \returns \c std::vector
   */
  std::vector<uint32_t> mblockread(uint32_t const& addr, uint32_t const& count)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    std::vector<uint32_t> data(count, 0);

    if (memhub_read(memsvc, addr, count, data.data()) != 0) {
      std::stringstream errmsg;
      errmsg << "blockread memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      throw std::runtime_error(errmsg.str());
    }
    return data;
  }

  /*!
   *  \brief Sequentially reads a block of values from the same raw register address.
   *         The address specified should behave like a port/FIFO.
   *         Register mask is not applied.
   *
   *  \throws \c std::runtime_error if the \c memhub_read has an nonzero exit status
   *
   *  \param \c addr address of the block to read
   *  \param \c count how many 32-bit words to read out of the block
   *
   *  \returns \c std::vector
   */
  std::vector<uint32_t> mfiforead(uint32_t const& addr, uint32_t const& count)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    std::vector<uint32_t> data(count, 0);

    for (auto && val : data) {
      if (memhub_read(memsvc, addr, 1, &val) != 0) {
        std::stringstream errmsg;
        errmsg << "fiforead memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
    }
    return data;
  }

  /*!
   *  \brief Reads a list of raw addresses
   *
   *  \throws \c std::runtime_error if the \c memhub_read has an nonzero exit status
   *
   *  \param \c reglist list of addresses to read
   *
   *  \returns \c std::vector list of register values read
   */
  std::vector<uint32_t> mlistread(std::vector<uint32_t> const& reglist)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    std::vector<uint32_t> data(reglist.size(), 0);

    for (size_t i = 0; i < reglist.size(); ++i) {
      if (memhub_read(memsvc, reglist.at(i), 1, &data.at(i)) != 0) {
        std::stringstream errmsg;
        errmsg << "listread memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
    }
    return data;
  }

  /*!
   *  \brief Writes a block of values to a contiguous memory block
   *
   *  \throws \c std::runtime_error if the \c memhub_write has an nonzero exit status
   *
   *  \param \c addr address of the start of the block
   *  \param \c data list of values to write into the block consecutively
   */
  void mblockwrite(uint32_t const& addr, std::vector<uint32_t> const& data)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    if (memhub_write(memsvc, addr, data.size(), data.data()) != 0) {
      std::stringstream errmsg;
      errmsg << "blockwrite memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      throw std::runtime_error(errmsg.str());
    }
    return;
  }


  /*!
   *  \brief Writes a set of values to an address that acts as a port or FIFO
   *
   *  \throws \c std::runtime_error if the \c memhub_write has an nonzero exit status
   *
   *  \param \c addr address of the FIFO port
   *  \param \c data list of values to write into the block consecutively
   */
  void mfifowrite(uint32_t const& addr, std::vector<uint32_t> const& data)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    for (auto && writeval : data) {
      if (memhub_write(memsvc, addr, 1, &writeval) != 0) {
        std::stringstream errmsg;
        errmsg << "fifowrite memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
    }
    return;
  }

  /*!
   *  \brief Writes a set of values to a list of addresses
   *
   *  \throws \c std::runtime_error if the \c memhub_write has an nonzero exit status
   *
   *  \param \c reglist map between address and value to be written
   */
  void mlistwrite(const std::unordered_map<uint32_t, uint32_t> &regvals)
  {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    for (auto && regval : regvals){
      if (memhub_write(memsvc, regval.first, 1, &(regval.second)) != 0) {
        std::stringstream errmsg;
        errmsg << "listwrite memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
    }
    return;
  }

}
// extern "C" {
//   const char *module_version_key = "extras v1.0.1";
//   const int module_activity_color = 4;

//   void module_init(ModuleManager *modmgr)
//   {
//     utils::initLogging();

//     if (memhub_open(&memsvc) != 0) {
//       auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
//       LOG4CPLUS_ERROR(logger, "Unable to connect to memory service: " << memsvc_get_last_error(memsvc));
//       LOG4CPLUS_ERROR(logger, "Unable to load module");
//       return;
//     }

//     xhal::common::rpc::registerMethod<extras::mfiforead>(modmgr);
//     xhal::common::rpc::registerMethod<extras::mblockread>(modmgr);
//     xhal::common::rpc::registerMethod<extras::mlistread>(modmgr);
//     xhal::common::rpc::registerMethod<extras::mfifowrite>(modmgr);
//     xhal::common::rpc::registerMethod<extras::mblockwrite>(modmgr);
//     xhal::common::rpc::registerMethod<extras::mlistwrite>(modmgr);
//   }
// }
