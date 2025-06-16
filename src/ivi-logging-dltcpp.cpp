#include "ivi-logging-dltcpp.h"

#include <sys/stat.h>
#include <thread>
#include <vector>

namespace logging::dlt {

static char const* dlt_daemon_fifo = "/tmp/dlt";
static char const* dltFifoBaseDir = "/tmp/dltpipes";

static auto const pid = getpid();

DaemonConnection& DaemonConnection::getInstance() {
    static DaemonConnection instance;
    instance.init();
    return instance;
}

static std::vector<DltCppContextClass*> contexts;

int32_t DaemonConnection::registerContext(DltCppContextClass& context) {
    uint32_t const pos = contexts.size();
    context.log_level_pos = pos;
    contexts.push_back(&context);

    initDaemonConnection();
    if (isDaemonConnected()) {
        sendContextRegistration(context);
    }

    return pos;
}

inline void assign_id(char* dest, char const* src) {
    memcpy(dest, src, DLT_ID_SIZE);
}

static constexpr int8_t DLT_USER_LOG_LEVEL_NOT_SET = -2;
static constexpr int8_t DLT_USER_TRACE_STATUS_NOT_SET = -2;

void DaemonConnection::sendContextRegistration(DltCppContextClass& context) {
    size_t const descriptionLength = strlen(context.m_context->getDescription());

    DltUserHeader userHeader{.message = MessageType::RegisterContext};
    DltUserControlMsgRegisterContext registerMessage{.log_level_pos = context.log_level_pos,
                                                     .log_level = DLT_USER_LOG_LEVEL_NOT_SET,
                                                     .trace_status = DLT_USER_TRACE_STATUS_NOT_SET,
                                                     .pid = pid,
                                                     .description_length = static_cast<uint32_t>(descriptionLength)};

    assign_id(registerMessage.apid, s_pAppLogContext->m_id.c_str());
    assign_id(registerMessage.ctid, context.m_context->getID());

    assign_id(context.extendedHeader.apid, s_pAppLogContext->m_id.c_str());     /* application id */
    assign_id(context.extendedHeader.ctid, context.getParentContext().getID()); /* context id */

    DaemonConnection::getInstance().send(userHeader, registerMessage, span<char>(context.m_context->getDescription(), descriptionLength));
}

void DaemonConnection::applyLogLevel(DltUserControlMsgLogLevel const& message) {
    contexts[message.log_level_pos]->setActiveLogLevel(static_cast<DltLogLevelType>(message.log_level));
}

void DaemonConnection::initDaemonConnection() {

    if (not isDaemonConnected()) {
        /* open DLT output FIFO */
        m_daemonFileDescriptor = open(dlt_daemon_fifo, O_WRONLY | O_NONBLOCK | O_CLOEXEC);

        if (isDaemonConnected()) {
            printf("Connection to the DLT daemon established\n");

            char descriptionWithPID[1024];
            uint32_t const descriptionLength =
                snprintf(descriptionWithPID, sizeof(descriptionWithPID), "PID:%i / %s", pid, s_pAppLogContext->m_description.c_str());

            DltUserHeader userHeader{.message = MessageType::RegisterApp};
            DltUserControlMsgRegisterApplication registerMesssage{.pid = pid, .description_length = descriptionLength};
            assign_id(registerMesssage.apid, s_pAppLogContext->m_id.c_str());

            send(userHeader, registerMesssage, span{descriptionWithPID, descriptionLength});

            for (auto const context : contexts) {
                sendContextRegistration(*context);
            }
        } else {
            //            printf("No connection to DLT daemon\n");
        }
    }
}

void DaemonConnection::sendLog(DltCppLogData& data) {
    initDaemonConnection();

    if (isDaemonConnected()) {
        auto const& context = *data.m_context;
        DltStandardHeader standardheader{};
        standardheader.mcnt = data.m_messageCount;

        DltStandardHeaderExtra headerextra{};
        headerextra.seid = htonl((uint32_t)pid);
        headerextra.tmsp = htonl(dlt_uptime());

        DltExtendedHeader extendedheader = context.extendedHeader;
        extendedheader.msin |= (uint8_t)((data.m_dltLogLevel << DLT_MSIN_MTIN_SHIFT) & DLT_MSIN_MTIN);
        extendedheader.noar = data.m_argsCount; /* number of arguments */

        uint16_t standardheaderLen =
            sizeof(DltStandardHeader) + sizeof(DltExtendedHeader) + DLT_STANDARD_HEADER_EXTRA_SIZE(standardheader.htyp) + data.m_contentSize;
        standardheader.len = htons(standardheaderLen);

        DaemonConnection::getInstance().send(data.sendLogUserHeader, standardheader, headerextra, extendedheader,
                                             span(data.m_content.data(), data.m_contentSize));
    }
}

template <typename... Types>
void DaemonConnection::send(Types const&... values) {
    std::array<iovec, sizeof...(Types)> buffers;
    auto* x = buffers.data();
    [[maybe_unused]] int unused[]{(to_iovec(*x++, values), 1)...};

    [[maybe_unused]] auto const bytes_written = writev(m_daemonFileDescriptor, buffers.data(), buffers.size());
    if (bytes_written < 0) {
        m_daemonFileDescriptor = disconnectedFromDaemonFd;
    }
}

void DaemonConnection::handleIncomingMessage() {
    std::array<char, 1024> buffer;
    int const byteCount = read(m_appFileDescriptor, buffer.data(), buffer.size());
    if (byteCount != -1) {
        IVILOGGING_DLT_DEBUG_TRACE("received \n%s", binaryToHex(buffer.data(), byteCount));

        auto pMessage = buffer.data();

        while (buffer.data() + byteCount > pMessage + sizeof(DltUserHeader)) {
            auto const userHeader = reinterpret_cast<DltUserHeader const*>(pMessage);
            pMessage += sizeof(DltUserHeader);

            switch (userHeader->message) {
                case MessageType::DLT_USER_MESSAGE_LOG_STATE: {
                    auto const logStateMsg [[maybe_unused]] = reinterpret_cast<DltUserControlMsgLogState const*>(pMessage);
                    pMessage += sizeof(DltUserControlMsgLogState);
                    IVILOGGING_DLT_DEBUG_INCOMING("received DltUserControlMsgLogState");
                } break;

                case MessageType::DLT_USER_MESSAGE_LOG_LEVEL: {
                    auto const logLevelMsg = reinterpret_cast<DltUserControlMsgLogLevel const*>(pMessage);
                    pMessage += sizeof(DltUserControlMsgLogLevel);
                    IVILOGGING_DLT_DEBUG_INCOMING("received DltUserControlMsgLogLevel %d pos:%d", logLevelMsg->log_level, logLevelMsg->log_level_pos);
                    applyLogLevel(*logLevelMsg);
                } break;

                default: {
                    return;
                } break;
            }
        }
    }
}

void DaemonConnection::init() {

    if (not m_initialized) {
        int ret = mkdir(dltFifoBaseDir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_ISVTX);

        if ((ret == -1) && (errno != EEXIST)) {
            std::cerr << "FIFO user dir " << dltFifoBaseDir << " cannot be created!\n";
            abort();
        }

        /* if dlt pipes directory is created by the application also chmod the directory */
        if (ret == 0) {
            /* S_ISGID cannot be set by mkdir, let's reassign right bits */
            ret = chmod(dltFifoBaseDir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_ISGID | S_ISVTX);

            if (ret == -1) {
                std::cerr << "FIFO user dir " << dltFifoBaseDir << " cannot be chmoded!\n";
                abort();
            }
        }

        char filename[DLT_PATH_MAX];

        /* create and open DLT user FIFO */
        snprintf(filename, DLT_PATH_MAX, "%s/dlt%d", dltFifoBaseDir, pid);

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

        m_appFileDescriptor = open(filename, O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (m_appFileDescriptor == DLT_FD_INIT) {
            assert(false);
            abort();
        }

        m_initialized = true;

        readerThread = std::thread([this]() {
            while (not m_stopRequested) {
                handleIncomingMessage();
                sleep(1);
            };
        });
    }
}

} // namespace logging::dlt
