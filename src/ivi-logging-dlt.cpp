#include "ivi-logging-dltcpp.h"

#include <sys/stat.h>

namespace logging::dlt {

static char const* dlt_daemon_fifo = "/tmp/dlt";
char dltFifoBaseDir[DLT_PATH_MAX] = "/tmp";

DaemonConnection getDaemon() {

    char filename[DLT_PATH_MAX];
    char dlt_user_dir[DLT_PATH_MAX];
    int ret;

    snprintf(dlt_user_dir, DLT_PATH_MAX, "%s/dltpipes", dltFifoBaseDir);
    ret = mkdir(dlt_user_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_ISVTX);

    if ((ret == -1) && (errno != EEXIST)) {
        std::cerr << "FIFO user dir " << dlt_user_dir << " cannot be created!\n";
        abort();
    }

    /* if dlt pipes directory is created by the application also chmod the directory */
    if (ret == 0) {
        /* S_ISGID cannot be set by mkdir, let's reassign right bits */
        ret = chmod(dlt_user_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_ISGID | S_ISVTX);

        if (ret == -1) {
            std::cerr << "FIFO user dir " << dlt_user_dir << " cannot be chmoded!\n";
            abort();
        }
    }

    /* create and open DLT user FIFO */
    snprintf(filename, DLT_PATH_MAX, "%s/dlt%d", dlt_user_dir, getpid());

    /* Try to delete existing pipe, ignore result of unlink */
    unlink(filename);

    ret = mkfifo(filename, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP);

    if (ret == -1) {
        std::cerr << "Logging disabled, FIFO user " << filename << " cannot be created!\n";
        abort();
    }

    /* S_IWGRP cannot be set by mkfifo (???), let's reassign right bits */
    ret = chmod(filename, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP);

    if (ret == -1) {
        std::cerr << "FIFO user %s cannot be chmoded!\n";
        assert(false);
    }

    auto const appFileDescriptor = open(filename, O_RDWR | O_NONBLOCK | O_CLOEXEC);

    if (appFileDescriptor == DLT_FD_INIT) {
        assert(false);
        abort();
    }

    /* open DLT output FIFO */
    auto const daemonFileDescriptor = open(dlt_daemon_fifo, O_WRONLY | O_NONBLOCK | O_CLOEXEC);

    static DaemonConnection connection{.daemonFileDescriptor = daemonFileDescriptor, .appFileDescriptor = appFileDescriptor};
    return connection;
}

} // namespace logging::dlt
