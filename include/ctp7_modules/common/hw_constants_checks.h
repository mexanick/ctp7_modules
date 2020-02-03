/*! \file
 *  \brief Header containing helper functions to check the hardware related constants.
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef COMMON_HW_CONSTANTS_CHECKS_H
#define COMMON_HW_CONSTANTS_CHECKS_H

#include "hw_constants.h"

/*!
 *  \brief This namespace holds checks for constants related to the GBT.
 */
namespace gbt {
    /*!
     *  \brief This function checks the phase parameter validity.
     *
     *  \throws \c std::range_error if the specified phase is outside the allowed range
     *
     *  \param \c phase Phase value to check
     */
    inline void checkPhase(uint8_t phase) {
        std::stringstream errmsg;
        if (phase < PHASE_MIN) {
            errmsg << "The phase parameter supplied (" << phase << ") is smaller than the minimum phase (" << PHASE_MIN << ").";
            throw std::range_error(errmsg.str());
        } else if (phase > PHASE_MAX) {
            errmsg << "The phase parameter supplied (" << phase << ") is bigger than the maximum phase (" << PHASE_MAX << ").";
            throw std::range_error(errmsg.str());
        }
        return;
    }
}

#endif
