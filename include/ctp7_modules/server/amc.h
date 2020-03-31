#ifndef SERVER_AMC_H
#define SERVER_AMC_H

namespace amc {

  /*!
   *  \brief Returns AMC FW version
   *
   *  \throws \c std::runtime_error in the case FW version is not 1.X or 3.X
   *
   *  \param \c caller_name name of method which called the FW version check FIXME necessary?
   *
   *  \returns the major FW version number
   */
  uint32_t fw_version_check(const char *caller_name);

}

#endif
