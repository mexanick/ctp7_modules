#include "memhub.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
// #include <stdio.h>  // FIXME necessary?
// #include <stdlib.h> // FIXME necessary?
// #include <sys/stat.h>  // FIXME necessary?
// #include <sys/types.h> // FIXME necessary?

#define SEM_NAME "/memhub"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define SEM_INIT 1

static sem_t *semaphore = nullptr;
static bool busy = false;

int memhub_open(memsvc_handle_t *handle)
{
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
    if (semaphore == nullptr) {
        semaphore = sem_open(SEM_NAME, O_CREAT, SEM_PERMS, SEM_INIT);
        int semval = 0;
        sem_getvalue(semaphore, &semval);
        if (semval > 1) {
            LOG4CPLUS_ERROR(logger, "Invalid semaphore value = "
                           << semval
                           << ". Probably it was messed up by a dying process."
                           << " Please clean up this semaphore (you can just delete /dev/shm/sem.memhub, I think)");
            exit(1);
        }
        LOG4CPLUS_INFO(logger, "Memhub initialized a semaphore. Current semaphore value = " << semval);
    }
    if (semaphore == SEM_FAILED) {
        LOG4CPLUS_ERROR(logger, "sem_open(3) error");
        perror("sem_open(3) error");
        // FIXME throw std::runtime_error(errmsg.str());
        exit(1);
    }

    // handle all signals in attempt to undo an active semaphore if the process is killed in the middle of a transaction..
    signal(SIGABRT, die);
    signal(SIGFPE, die);
    signal(SIGILL, die);
    signal(SIGINT, die);
    signal(SIGSEGV, die);
    signal(SIGTERM, die);

    return memsvc_open(handle);
}

int memhub_close(memsvc_handle_t *handle)
{
    sem_close(semaphore);
    return memsvc_close(handle);
}

int memhub_read(memsvc_handle_t handle, uint32_t addr, uint32_t words, uint32_t *data)
{
    sem_wait(semaphore);
    busy = true;
    int ret = memsvc_read(handle, addr, words, data);
    sem_post(semaphore);
    busy = false;
    return ret;
}

int memhub_write(memsvc_handle_t handle, uint32_t addr, uint32_t words, const uint32_t *data)
{
    sem_wait(semaphore);
    busy = true;
    int ret = memsvc_write(handle, addr, words, data);
    sem_post(semaphore);
    busy = false;
    return ret;
}

void die(int signo)
{
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
    int semval = 0;
    sem_getvalue(semaphore, &semval);

    if (busy && (semval == 0)) {
        LOG4CPLUS_ERROR(logger, "[!] Application is dying, trying to undo an active semaphore...");
        sem_post(semaphore);
    }
    LOG4CPLUS_ERROR(logger, "[!] Application was killed or died with signal "
                    << signo << " (semaphore value at the time of the kill = " << semval << ")...");
    // FIXME throw std::runtime_error(errmsg.str());
    exit(1);
}
