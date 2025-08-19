#include "ivi-logging-dltcpp.h"

#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <thread>
#include <vector>

// #define IVILOGGING_DLT_DEBUG_INCOMINGDEBUG_IPC_TRACE(format, ...) printf("" format "\n", ##__VA_ARGS__)
// #define IVILOGGING_DLT_DEBUG_INCOMING(format, ...) printf("%s:%d %s     " format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define IVILOGGING_DLT_DEBUG_INCOMING(format, ...)
// #define IVILOGGING_DLT_DEBUG_TRACE(format, ...) printf("IVI: %s:%d %s" format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
// #define IVILOGGING_DLT_DEBUG_TRACE(format, ...) printf("IVI: %s:%d %s" format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define IVILOGGING_DLT_DEBUG_TRACE(format, ...)
// #define IVILOGGING_DLT_DEBUG_IPC_TRACE(format, ...) printf(format "\n", ##__VA_ARGS__)
#define IVILOGGING_DLT_DEBUG_IPC_TRACE(format, ...)

namespace logging::dlt {

static char const* dlt_daemon_fifo = "/tmp/dlt";
static char const* dltFifoBaseDir = "/tmp/dltpipes";

static auto const pid = getpid();

template <typename Type>
class span {
  public:
    span(Type const* data, size_t size) : m_data{data}, m_size{size} {
    }

    Type const* data() const {
        return m_data;
    }

    size_t size() const {
        return m_size;
    }

    Type const* m_data;
    size_t m_size;
};

inline char printableAscii(char c) {
    if ((c > 32) && (c < 126)) {
        return c;
    } else
        return '.';
}

inline char* binaryToHex(void const* inSt, size_t inSize) {
    static thread_local char out[65535];
    auto const inStr = reinterpret_cast<unsigned char const*>(inSt);
    static char hex[] = "0123456789ABCDEF";
    size_t outIndex = 0;
    for (size_t i = 0; i < inSize; i++) {
        out[outIndex++] = hex[inStr[i] >> 4];
        out[outIndex++] = hex[inStr[i] & 0xF];
        out[outIndex++] = ' ';
    }

    out[outIndex++] = ' ';
    out[outIndex++] = '|';
    out[outIndex++] = ' ';

    for (size_t i = 0; i < inSize; i++) {
        out[outIndex++] = printableAscii(inStr[i]);
        out[outIndex++] = ' ';
    }

    out[outIndex++] = 0;
    return out;
}

template <typename Type>
void to_iovec(iovec& iovec, Type const& type) {
    iovec.iov_base = const_cast<Type*>(&type);
    iovec.iov_len = sizeof(type);
    IVILOGGING_DLT_DEBUG_IPC_TRACE("IVI: %s", binaryToHex(iovec.iov_base, iovec.iov_len));
}

template <typename Type>
void to_iovec(iovec& iovec, span<Type> const& buffer) {
    iovec.iov_base = const_cast<Type*>(buffer.data());
    iovec.iov_len = buffer.size();
    IVILOGGING_DLT_DEBUG_IPC_TRACE("IVI: %s", binaryToHex(iovec.iov_base, iovec.iov_len));
}

uint32_t dlt_uptime(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint32_t)ts.tv_sec * 10000 + (uint32_t)ts.tv_nsec / 100000; /* in 0.1 ms = 100 us */
    else
        return 0;
}

DaemonConnection::DaemonConnection() {
    pipe(m_stopPipe);
}

DaemonConnection& DaemonConnection::getInstance() {
    static DaemonConnection instance;
    instance.init();
    return instance;
}

void DltCppLogData::addMessageTooLargeIndication() {
    if (not m_isFull) {
        MessageTooLargeData data;
        memcpy(m_content.data() + m_contentSize, &data, sizeof(data));
        m_contentSize += sizeof(data);

        m_argsCount++;

        m_isFull = true;
        printf("DLT message too large\n");
    }
}

DltCppLogData::~DltCppLogData() {

    if (isEnabled()) {
        if (m_context->isSourceCodeLocationInfoEnabled()) {
            write("                                                    | ");
            if (getData().getFileName() != nullptr)
                write(getData().getFileName());
            if (getData().getLineNumber() != -1)
                write(getData().getLineNumber());
            if (getData().getPrettyFunction() != nullptr)
                write(getData().getPrettyFunction());
        }

        if (m_context->isThreadInfoEnabled()) {
            write("ThreadID");
            write(getThreadInformation().getID());
            write(getThreadInformation().getName());
        }

        DaemonConnection::getInstance().sendLog(*this);
    }
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
        m_daemonFileDescriptor = open(dlt_daemon_fifo, O_WRONLY | O_CLOEXEC);

        if (isDaemonConnected()) {
            printf("Connection to the DLT daemon established\n");

            char descriptionWithPID[1024];
            uint32_t const descriptionLength =
                snprintf(descriptionWithPID, sizeof(descriptionWithPID), "%s / PID:%i", s_pAppLogContext->m_description.c_str(), pid);

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
        printf("Could not write data to DLT daemon socket\n");
        close(m_daemonFileDescriptor);
        m_daemonFileDescriptor = disconnectedFromDaemonFd;
    }
}

void DaemonConnection::handleIncomingMessage() {

    std::array<struct pollfd, 2> pfds{};
    pfds[0].fd = m_appFileDescriptor;
    pfds[0].events = POLLIN | POLLHUP | POLLNVAL | POLLERR | POLLRDHUP;
    pfds[1].fd = m_stopPipe[0];
    pfds[1].events = POLLIN | POLLHUP | POLLNVAL | POLLERR | POLLRDHUP;

    auto i = poll(pfds.data(), pfds.size(), -1);

    if (i > 0) {
        if (pfds[0].revents == POLLIN) {
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
            // Ensure that our thread does not catch any signal
            sigset_t signal_mask{};
            sigfillset(&signal_mask);
            auto rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
            assert(rc == 0);

            while (not m_stopRequested) {
                handleIncomingMessage();
            };
        });
    }
}

DaemonConnection::~DaemonConnection() {
    m_stopRequested = true;
    write(m_stopPipe[1], &m_stopRequested, sizeof(m_stopRequested));
    readerThread.join();
}

void DltCppContextClass::setActiveLogLevel(DltLogLevelType activeLogLevel) {
    m_activeLogLevel = activeLogLevel;
    IVILOGGING_DLT_DEBUG_TRACE("Log level for context: %s set to %d", this->m_context->getID(), activeLogLevel);
}

void DltCppLogData::init(DltCppContextClass& context, LogInfo const& data) {
    m_data = &data;
    m_context = &context;
    m_dltLogLevel = m_context->getDLTLogLevel(getData().getLogLevel());
    m_enabled = context.isEnabled(getData().getLogLevel());

    m_messageCount = context.messageCounter();

#ifndef NDEBUG
    m_content.fill(-1);
#endif
}

} // namespace logging::dlt
