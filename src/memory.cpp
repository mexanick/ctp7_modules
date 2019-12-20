#include "ctp7_modules/server/moduleapi.h"
#include <libmemsvc.h>
#include "ctp7_modules/server/memhub.h"

#include "xhal/common/rpc/register.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <sstream>

memsvc_handle_t memsvc;

namespace memory {
    struct mread : public xhal::common::rpc::Method
    {
        std::vector<uint32_t> operator()(const uint32_t &addr, const uint32_t &count) const;
    };

    struct mwrite : public xhal::common::rpc::Method
    {
        void operator()(const uint32_t &addr, const std::vector<uint32_t> &data) const;
    };
};

std::vector<uint32_t> memory::mread::operator()(const uint32_t &addr, const uint32_t &count) const
{
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    std::vector<uint32_t> data(count);

    if (memhub_read(memsvc, addr, count, data.data()) == 0) {
        return data;
    } else {
        std::stringstream errmsg;
        errmsg << "read memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
    }
}

void memory::mwrite::operator()(const uint32_t &addr, const std::vector<uint32_t> &data) const
{
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    if (memhub_write(memsvc, addr, data.size(), data.data()) != 0) {
        std::stringstream errmsg;
        errmsg << "write memsvc error: " << memsvc_get_last_error(memsvc);
        LOG4CPLUS_ERROR(logger, errmsg.str());
        throw std::runtime_error(errmsg.str());
    }
}

extern "C" {
    const char *module_version_key = "memory v1.0.1";
    const int module_activity_color = 4;

    void module_init(ModuleManager *modmgr)
    {
        if (memhub_open(&memsvc) != 0) {
            auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
            LOG4CPLUS_ERROR(logger, "Unable to connect to memory service: " << memsvc_get_last_error(memsvc));
            LOG4CPLUS_ERROR(logger, "Unable to load module");
            return;
        }
        xhal::common::rpc::registerMethod<memory::mread>(modmgr);
        xhal::common::rpc::registerMethod<memory::mwrite>(modmgr);
    }
}
