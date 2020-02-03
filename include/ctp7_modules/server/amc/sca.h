#ifndef SERVER_AMC_SCA_H
#define SERVER_AMC_SCA_H

namespace amc {
  namespace sca {

    /*!
     * \brief Prepare data for use with the SCA communication interfaces
     *
     * \details SCA TX/RX data is transmitted using the HDLC protocol, which is 16-bit length, and sent LSB to MSB.
     *          In the HDLC packet, it is sent/received as [<16:31><0:15>].
     *          The GEM_AMC firmware stores it as [<7:0><15:8><23:16><31:24>]
     *
     * \param \c data is the data to be converted to the appropriate ordering
     */
    uint32_t formatSCAData(uint32_t const& data);

  }
}

#endif
