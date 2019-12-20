#ifndef BLASTER_RAM_DEFS_H
#define BLASTER_RAM_DEFS_H

class BLASTERSettings {
 public:

  /*!
   * \brief BLASTER RAM defs
   */
  struct BLASTERType {
    enum EBLASTERType {
      GBT        = 0x1, ///< GBT RAM
      OptoHybrid = 0x2, ///< OptoHybrid RAM
      VFAT       = 0x4, ///< VFAT RAM
      ALL        = 0x7, ///< All RAMs
    } BLASTERType;

    /* /\*! */
    /*  * \brief With its intrusive serializer */
    /*  *\/ */
    /* template<class Message> void serialize(Message & msg) { */
    /*   msg & BLASTERType; */
    /* } */
  };
};

// <name>  is the enum scoped namespace for scope::VALUE access
// <name>T is the enum type

// typedef the struct for access to the members via struct::VALUE
typedef BLASTERSettings::BLASTERType  BLASTERType;


// typedef the enum for casting and access
typedef BLASTERSettings::BLASTERType::EBLASTERType  BLASTERTypeT;

namespace xhal {
  namespace common {
    namespace rpc {
      template<typename Message>
      inline void serialize(Message &msg, BLASTERTypeT &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, BLASTERType &value) {
        msg & value;
      }
    }
  }
}

#endif
