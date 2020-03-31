#ifndef COMMON_BLASTER_RAM_DEFS_H
#define COMMON_BLASTER_RAM_DEFS_H

#include <stdint.h>

namespace amc {
  namespace blaster {

    /*!
     * \brief BLASTER RAM defs
     */
    enum struct BLASTERType : uint8_t {
      GBT        = 0x1, ///< GBT RAM
      OptoHybrid = 0x2, ///< OptoHybrid RAM
      VFAT       = 0x4, ///< VFAT RAM
      ALL        = 0x7, ///< All RAMs
    };
  }
}

namespace xhal {
  namespace common {
    namespace rpc {
      template<typename Message>
      inline void serialize(Message &msg, amc::blaster::BLASTERType &value) {
        msg & value;
      }
    }
  }
}

#endif
