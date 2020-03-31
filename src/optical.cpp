#include <libwisci2c.h>
#include <cardconfig.h>
#include <libmemsvc.h>

#include "xhal/common/rpc/register.h"
#include "xhal/extern/ModuleManager.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

memsvc_handle_t memsvc;

namespace optical {
    struct measure_input_power : public xhal::common::rpc::Method
    {
        std::map<std::string, std::vector<uint32_t>> operator()() const;
    };
};

std::map<std::string, std::vector<uint32_t>> optical::measure_input_power::operator()() const
{
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

#ifndef WISCPARAM_SERIES_CTP7
    // This is only written for the CTP7 at the moment.
    LOG4CPLUS_ERROR(logger, "Unsupported function. CTP7 Only");
    throw std::runtime_error("Unsupported function. CTP7 Only");;
#endif

    std::map<std::string, std::vector<uint32_t>> power_readings;
    for (int i = 0; i < 3; ++i) {
        const std::string cxpID = "CXP" + std::to_string(i);
        char devnode[16];
        snprintf(devnode, 16, "/dev/i2c-%d", 2+i);
        int i2cfd = open(devnode, O_RDWR);
        if (i2cfd < 0) {
            std::stringstream errmsg;
            errmsg << "Unable to open " << devnode;
            LOG4CPLUS_ERROR(logger, errmsg.str());
            close(i2cfd);
            throw std::runtime_error(errmsg.str());
        }

        if (i2c_write(i2cfd, 0x54, 127, reinterpret_cast<const uint8_t*>("\x01"), 1) != 1) {
            std::stringstream errmsg;
            errmsg << "i2c write failure";
            LOG4CPLUS_ERROR(logger, errmsg.str());
            close(i2cfd);
            throw std::runtime_error(errmsg.str());
        }

        for (int j = 0; j < 12; ++j) {
            uint16_t power;
            if (i2c_read(i2cfd, 0x54, 206+(2*j), reinterpret_cast<uint8_t*>(&power), 2) != 2) {
              std::stringstream errmsg;
              errmsg << "i2c read failure";
              LOG4CPLUS_ERROR(logger, errmsg.str());
              close(i2cfd);
              throw std::runtime_error(errmsg.str());
            }
            power = __builtin_bswap16(power);
            LOG4CPLUS_INFO(logger, "raw value: 0x"
                           << std::hex << std::setw(4) << std::setfill('0') << power
                           << " = " << power << " / 10 = " << (power/10));
            power_readings[cxpID].push_back(power/10);
        }

        close(i2cfd);
    }

    int i2cfd = open("/dev/i2c-1", O_RDWR);
    if (i2cfd < 0) {
        std::stringstream errmsg;
        errmsg << "Unable to open i2c-1";
        LOG4CPLUS_ERROR(logger, errmsg.str());
        close(i2cfd);
        throw std::runtime_error(errmsg.str());
    }

    for (int i = 0; i < 3; ++i) {
        const std::string mpID = "MP" + std::to_string(i);
        for (int j = 0; j < 12; ++j) {
            uint16_t power;
            if (i2c_read(i2cfd, 0x30+i, 64+(2*j), reinterpret_cast<uint8_t*>(&power), 2) != 2) {
                std::stringstream errmsg;
                errmsg << "i2c read failure";
                LOG4CPLUS_ERROR(logger, errmsg.str());
                close(i2cfd);
                throw std::runtime_error(errmsg.str());
            }
            power = __builtin_bswap16(power);
            power_readings[mpID].push_back(power/10);
        }

    }
    close(i2cfd);

    return power_readings;
}

extern "C" {
    const char *module_version_key = "optical v1.0.0";
    const int module_activity_color = 0;

    void module_init(ModuleManager *modmgr)
    {
        xhal::common::rpc::registerMethod<optical::measure_input_power>(modmgr);
    }
}
