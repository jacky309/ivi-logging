#pragma once

#include "dlt_user.h"
#include "ivi-logging-common.h"
#include <array>
#include <cstring>
#include <sys/time.h>
#include <sys/uio.h>
#include <thread>

// #define DEBUG_TRACE(format, ...) printf("%s:%d %s     " format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define DEBUG_TRACE(format, ...)

namespace logging {

namespace dlt {

enum class MessageType : uint32_t { SendLog = 1, RegisterApp = 2, RegisterContext = 4, DLT_USER_MESSAGE_LOG_STATE = 12, DLT_USER_MESSAGE_LOG_LEVEL = 6 };

/**
 * This is the internal message content to exchange control msg log level information between application and daemon.
 */
typedef struct {
    uint8_t log_level;     /**< log level */
    uint8_t trace_status;  /**< trace status */
    int32_t log_level_pos; /**< offset in management structure on user-application side */
} DLT_PACKED DltUserControlMsgLogLevel;

typedef struct {
    int8_t log_state; /**< the state to be used for logging state: 0 = off, 1 = external client connected */
} DLT_PACKED DltUserControlMsgLogState;

typedef struct {
    char pattern[DLT_ID_SIZE]{'D', 'U', 'H', 1}; /**< This pattern should be DUH0x01 */
    MessageType message;                         /**< messsage info */
} DLT_PACKED DltUserHeader;

typedef struct {
    char apid[DLT_ID_SIZE];      /**< application id */
    pid_t pid;                   /**< process id of user application */
    uint32_t description_length; /**< length of description */
} DLT_PACKED DltUserControlMsgRegisterApplication;

typedef struct {
    char apid[DLT_ID_SIZE];      /**< application id */
    char ctid[DLT_ID_SIZE];      /**< context id */
    int32_t log_level_pos{};     /**< offset in management structure on user-application side */
    int8_t log_level{};          /**< log level */
    int8_t trace_status{};       /**< trace status */
    pid_t pid;                   /**< process id of user application */
    uint32_t description_length; /**< length of description */
} DLT_PACKED DltUserControlMsgRegisterContext;

typedef struct {
    char pattern[DLT_ID_SIZE]{'D', 'L', 'T', 1}; /**< This pattern should be DLT0x01 */
    uint32_t seconds;                            /**< seconds since 1.1.1970 */
    int32_t microseconds;                        /**< Microseconds */
    char ecu[DLT_ID_SIZE]{'E', 'C', 'U', '1'};   /**< The ECU id is added, if it is not already in the DLT message itself */
} DLT_PACKED DltStorageHeader;

/**
 * The structure of the DLT standard header. This header is used in each DLT message.
 */
typedef struct {
    uint8_t htyp{DLT_HTYP_PROTOCOL_VERSION1 | DLT_HTYP_WEID | DLT_HTYP_WTMS | DLT_HTYP_WSID |
                 DLT_HTYP_UEH}; /**< This parameter contains several informations, see definitions below */
    uint8_t mcnt;               /**< The message counter is increased with each sent DLT message */
    uint16_t len;               /**< Length of the complete message, without storage header */
} DLT_PACKED DltStandardHeader;

/**
 * The structure of the DLT extra header parameters. Each parameter is sent only if enabled in htyp.
 */
typedef struct {
    char ecu[DLT_ID_SIZE]{'E', 'C', 'U', '1'}; /**< ECU id */
    uint32_t seid{};                           /**< Session number */
    uint32_t tmsp;                             /**< Timestamp since system start in 0.1 milliseconds */
} DLT_PACKED DltStandardHeaderExtra;

/**
 * The structure of the DLT extended header. This header is only sent if enabled in htyp parameter.
 */
typedef struct {
    uint8_t msin{DLT_MSIN_VERB | (DLT_TYPE_LOG << DLT_MSIN_MSTP_SHIFT)}; /**< messsage info */
    uint8_t noar;                                                        /**< number of arguments */
    char apid[DLT_ID_SIZE];                                              /**< application id */
    char ctid[DLT_ID_SIZE];                                              /**< context id */
} DLT_PACKED DltExtendedHeader;

static constexpr auto ENABLE_DEBUG = false;
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

class DltCppLogData;

/**
 * Call the given function with each element of the given tuple
 */
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline void for_each(std::tuple<Tp...>& t, FuncT f) {
    if constexpr (I < sizeof...(Tp)) {
        f.template operator()<I>(std::get<I>(t));
        for_each<I + 1, FuncT, Tp...>(t, f);
    }
}

struct pass {
    template <typename... T>
    pass(T...) {
    }
};

template <typename... Args>
void h(Args... args) {
    size_t const nargs = sizeof...(args); // get number of args
    iovec x_array[nargs];                 // create X array of that size

    iovec* x = x_array;
    int unused[]{(f(*x++, args), 1)...}; // call f
    pass{unused};

    g(x_array, nargs); // call g with x_array
}

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

    if (ENABLE_DEBUG) {
        printf("IVI: %s\n", binaryToHex(iovec.iov_base, iovec.iov_len));
    }
}

template <typename Type>
void to_iovec(iovec& iovec, span<Type> const& type) {
    iovec.iov_base = const_cast<Type*>(type.data());
    iovec.iov_len = type.size();
    if (ENABLE_DEBUG) {
        printf("IVI: %s\n", binaryToHex(iovec.iov_base, iovec.iov_len));
    }
}

class DltCppContextClass;
class DaemonConnection {
  public:
    static DaemonConnection& getInstance();

    ~DaemonConnection() {
        m_stopRequested = true;
        readerThread.join();
    }

    template <typename... Types>
    void send(Types const&... values) {
        std::array<iovec, sizeof...(Types)> buffers;
        auto* x = buffers.data();
        [[maybe_unused]] int unused[]{(to_iovec(*x++, values), 1)...};

        [[maybe_unused]] auto const bytes_written = writev(daemonFileDescriptor, buffers.data(), buffers.size());
    }

    void handleIncomingMessage() {
        std::array<char, 1024> buffer;
        int const byteCount = read(appFileDescriptor, buffer.data(), buffer.size());
        if (byteCount != -1) {
            DEBUG_TRACE("received %s", binaryToHex(buffer.data(), byteCount));
            assert(static_cast<size_t>(byteCount) >= sizeof(DltUserHeader));
            auto const userHeader = reinterpret_cast<DltUserHeader const*>(buffer.data());
            void const* message = buffer.data() + sizeof(DltUserHeader);

            switch (userHeader->message) {
                case MessageType::DLT_USER_MESSAGE_LOG_STATE: {
                    //                    auto const logLevelMsg = reinterpret_cast<DltUserControlMsgLogState const*>(message);
                    DEBUG_TRACE("received DltUserControlMsgLogState");
                } break;

                case MessageType::DLT_USER_MESSAGE_LOG_LEVEL: {
                    auto const logLevelMsg = reinterpret_cast<DltUserControlMsgLogLevel const*>(message);
                    DEBUG_TRACE("received DltUserControlMsgLogLevel %d pos:%d", logLevelMsg->log_level, logLevelMsg->log_level_pos);
                    applyLogLevel(*logLevelMsg);
                }

                default: {
                } break;
            }
        }
    }

    int32_t registerContext(DltCppContextClass* context);

    void applyLogLevel(DltUserControlMsgLogLevel const& message);

  private:
    void init();

    std::thread readerThread;

    int daemonFileDescriptor;
    int appFileDescriptor;
    bool m_initialized{false};
    bool m_stopRequested{false};
};

inline void assign_id(char* dest, char const* src) {
    memcpy(dest, src, DLT_ID_SIZE);
}

class DltCppContextClass : public LogContextBase {

  public:
    using LogDataType = DltCppLogData;

    DltCppContextClass() {
    }

    ~DltCppContextClass() {
    }

    void setParentContext(LogContextCommon& context) {
        m_context = &context;
        DEBUG_TRACE("m_context = %p", (void*)m_context);
    }

    bool isEnabled(LogLevel logLevel) const {
        auto const dltLogLevel = getDLTLogLevel(logLevel);
        return dltLogLevel <= m_activeLogLevel;
    }

    auto messageCounter() {
        void const* pMessageCount = &m_messageCount;
        DEBUG_TRACE("pMessageCount = %p", pMessageCount);
        return m_messageCount.fetch_add(1);
    }

    static DltLogLevelType getDLTLogLevel(LogLevel level) {
        DltLogLevelType v = DLT_LOG_DEFAULT;
        switch (level) {
            case LogLevel::Debug:
                v = DLT_LOG_DEBUG;
                break;
            case LogLevel::Info:
                v = DLT_LOG_INFO;
                break;
            case LogLevel::Warning:
                v = DLT_LOG_WARN;
                break;
            case LogLevel::Fatal:
                v = DLT_LOG_FATAL;
                break;
            case LogLevel::Error:
                v = DLT_LOG_ERROR;
                break;
            case LogLevel::Verbose:
                v = DLT_LOG_VERBOSE;
                break;
            case LogLevel::None:
                v = DLT_LOG_OFF;
                break;
            default:
                v = DLT_LOG_DEFAULT;
                break;
        }
        return v;
    }

    auto const& getParentContext() const {
        return *m_context;
    }

    void registerContext() {
        LogContextBase::registerContext();

        if (!isDLTAppRegistered()) {
            registerDLTApp(s_pAppLogContext->m_id.c_str(), s_pAppLogContext->m_description.c_str());
            isDLTAppRegistered() = true;
        }

        size_t const descriptionLength = strlen(m_context->getDescription());

        DltUserHeader userHeader{.message = MessageType::RegisterContext};
        DltUserControlMsgRegisterContext registerMessage{.log_level_pos = DaemonConnection::getInstance().registerContext(this),
                                                         .pid = getpid(),
                                                         .description_length = static_cast<uint32_t>(descriptionLength)};

        assign_id(registerMessage.apid, s_pAppLogContext->m_id.c_str());
        assign_id(registerMessage.ctid, m_context->getID());

        DaemonConnection::getInstance().send(userHeader, registerMessage, span<char>(m_context->getDescription(), descriptionLength));
        assign_id(extendedHeader.apid, s_pAppLogContext->m_id.c_str()); /* application id */
        assign_id(extendedHeader.ctid, getParentContext().getID());     /* context id */
    }

    static bool& isDLTAppRegistered() {
        static bool m_appRegistered = false;
        return m_appRegistered;
    }

    /**
     * Register the application.
     */
    static void registerDLTApp(char const* id, char const* description) {
        pid_t pid = getpid();
        char descriptionWithPID[1024];
        uint32_t const descriptionLength = snprintf(descriptionWithPID, sizeof(descriptionWithPID), "PID:%i / %s", pid, description);

        DltUserHeader userHeader{.message = MessageType::RegisterApp};
        DltUserControlMsgRegisterApplication registerMesssage{.pid = pid, .description_length = descriptionLength};
        assign_id(registerMesssage.apid, id);

        DaemonConnection::getInstance().send(userHeader, registerMesssage, span{descriptionWithPID, descriptionLength});
    }

    void setActiveLogLevel(DltLogLevelType activeLogLevel) {
        m_activeLogLevel = activeLogLevel;
        DEBUG_TRACE("Log level for context: %s set to %d", this->m_context->getID(), activeLogLevel);
    }

  private:
    LogContextCommon* m_context = nullptr;
    DltExtendedHeader extendedHeader{};
    std::atomic<int> m_messageCount{0};

    DltLogLevelType m_activeLogLevel{DltLogLevelType::DLT_LOG_MAX};

    friend class DltCppLogData;
};

class DltCppLogData : public ::logging::LogData {

  public:
    using ContextType = DltCppContextClass;

    DltCppLogData() {
        DEBUG_TRACE("DltCppLogData() this= %p", (void*)this);
    }

    void init(DltCppContextClass& context, LogInfo const& data) {
        m_data = &data;
        m_context = &context;
        m_dltLogLevel = m_context->getDLTLogLevel(getData().getLogLevel());
        m_enabled = context.isEnabled(getData().getLogLevel());

        DEBUG_TRACE("m_context = %p", (void*)m_context);

        /*
                struct timeval tv;
                gettimeofday(&tv, NULL);
                m_storageHeader.seconds = (uint32_t)tv.tv_sec;
                m_storageHeader.microseconds = (int32_t)tv.tv_usec;
        */
    }

    virtual ~DltCppLogData() {
        DEBUG_TRACE("DltCppLogData() this= %p ,  m_context = %p", (void*)this, (void*)m_context);

        if (isEnabled()) {
            if (m_context->isSourceCodeLocationInfoEnabled()) {
                /*
                dlt_user_log_write_utf8_string(this, "                                                    | ");
                if (getData().getFileName() != nullptr)
                    dlt_user_log_write_utf8_string(this, getData().getFileName());
                if (getData().getLineNumber() != -1)
                    dlt_user_log_write_uint32(this, getData().getLineNumber());
                if (getData().getPrettyFunction() != nullptr)
                    dlt_user_log_write_utf8_string(this, getData().getPrettyFunction());
                    */
            }

            if (m_context->isThreadInfoEnabled()) {
                /*
                dlt_user_log_write_string(this, "ThreadID");
                dlt_user_log_write_uint8(this, getThreadInformation().getID());
                dlt_user_log_write_string(this, getThreadInformation().getName());
                */
            }

            sendLog();
        }
    }

    uint32_t dlt_uptime(void) {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
            return (uint32_t)ts.tv_sec * 10000 + (uint32_t)ts.tv_nsec / 100000; /* in 0.1 ms = 100 us */
        else
            return 0;
    }

    void sendLog() {
        DEBUG_TRACE("m_context = %p\n", (void*)m_context);

        DltUserHeader userHeader{.message = MessageType::SendLog};

        DltStandardHeader standardheader{};
        standardheader.mcnt = m_context->messageCounter();

        DltStandardHeaderExtra headerextra{};
        headerextra.seid = (uint32_t)getpid();
        headerextra.tmsp = dlt_uptime();

        DltExtendedHeader extendedheader = m_context->extendedHeader;
        extendedheader.msin |= (uint8_t)((m_dltLogLevel << DLT_MSIN_MTIN_SHIFT) & DLT_MSIN_MTIN);
        extendedheader.noar = args_num; /* number of arguments */

        uint16_t standardheaderLen =
            sizeof(DltStandardHeader) + sizeof(DltExtendedHeader) + DLT_STANDARD_HEADER_EXTRA_SIZE(standardheader.htyp) + m_contentSize;
        standardheader.len = htons(standardheaderLen);

        DaemonConnection::getInstance().send(userHeader, standardheader, headerextra, extendedheader, span(m_content.data(), m_contentSize));

        DEBUG_TRACE("m_context = %p", (void*)m_context);
    }

    LogInfo const& getData() const {
        return *m_data;
    }

    bool isEnabled() const {
        return m_enabled;
    }

    bool isHexEnabled() const {
        return m_data->isHexEnabled();
    }

    template <typename... Args>
    void writeFormatted(char const* format, Args... args) {
        if (m_enabled) {
            std::array<char, 65536> buffer;

#pragma GCC diagnostic push
            // Make sure GCC does not complain about not being able to check the format string since it is no literal string
#pragma GCC diagnostic ignored "-Wformat-security"
            snprintf(buffer.data(), std::tuple_size_v<decltype(buffer)>, format, args...);
#pragma GCC diagnostic pop

            write(buffer.data());
        }
    }

    void write(bool v) {
        writeWithTypeInfo(DLT_TYPE_INFO_BOOL, v);
    }

    void write(char const* v) {
        write(v, strlen(v));
    }

    bool checkOverflow(size_t additionalSize) {
        auto const newSize = m_contentSize + additionalSize;
        if (newSize >= m_content.size()) {
            m_contentSize = 0;
            write("DLT message too large");
            m_isFull = true;
        }
        return not m_isFull;
    }

    void writeBuffer(void const* v, size_t size) {
        if (not m_isFull) {
            memcpy(m_content.data() + m_contentSize, v, size);
            m_contentSize += size;
        }
    }

    using DltTypeInfo = uint32_t;

    void write(char const* v, size_t size) {
        if (checkOverflow(size + 1 + sizeof(uint16_t) + sizeof(DltTypeInfo) )) {
            writeType(DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8);
            uint16_t const sizeAsUint16 = static_cast<uint16_t>(size) + 1;
            writeBuffer(&sizeAsUint16, sizeof(sizeAsUint16));
            writeBuffer(v, size + 1);
        }
    }

    void write(std::string_view v) {
        write(std::string{v});
    }

    void write(std::string const& v) {
        write(v.c_str(), v.size());
    }

    void write(float f) {
        // we assume a float is 32 bits
        writeWithTypeInfo(DLT_TYPE_INFO_FLOA | DLT_TYLE_32BIT, static_cast<float32_t>(f));
    }

// TODO : strangely, it seems like none of the types defined in "stdint.h" is equivalent to "long int" on a 32 bits platform
#if __WORDSIZE == 32
    void write(long int v) {
        writeWithTypeInfo(DLT_TYPE_INFO_SINT | DLT_TYLE_32BIT, v);
    }
    void write(unsigned long int v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT | (isHexEnabled() ? DLT_SCOD_HEX : 0), v);
    }
#endif

    void write(double f) {
        // we assume a double is 64 bits
        writeWithTypeInfo(DLT_TYPE_INFO_FLOA | DLT_TYLE_64BIT, static_cast<float64_t>(f));
    }

    void write(uint64_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_64BIT | (isHexEnabled() ? DLT_SCOD_HEX : 0), v);
    }

    void write(int64_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_SINT | DLT_TYLE_64BIT, v);
    }

    void write(uint32_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT | (isHexEnabled() ? DLT_SCOD_HEX : 0), v);
    }

    void write(int32_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT, v);
    }

    void write(uint16_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_16BIT | (isHexEnabled() ? DLT_SCOD_HEX : 0), v);
    }

    void write(int16_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_SINT | DLT_TYLE_16BIT, v);
    }

    void write(uint8_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_UINT | DLT_TYLE_8BIT | (isHexEnabled() ? DLT_SCOD_HEX : 0), v);
    }

    void write(int8_t v) {
        writeWithTypeInfo(DLT_TYPE_INFO_SINT | DLT_TYLE_8BIT, v);
    }

    template <typename Type>
    void writeWithTypeInfo(uint32_t type, Type const& value) {
        if (checkOverflow(sizeof(value) + sizeof(DltTypeInfo))) {
            writeType(type);
            writeBuffer(&value, sizeof(value));
        }
    }

    void writeType(uint32_t type) {
        writeBuffer(&type, sizeof(type));
        args_num++;
    }

  private:
    std::array<char, 2048> m_content;
    size_t m_contentSize{0};

    //    DltStorageHeader m_storageHeader;

    DltCppContextClass* m_context{};
    LogInfo const* m_data{};

    DltLogLevelType m_dltLogLevel;
    bool m_enabled{false};
    uint8_t args_num = 0;

    bool m_isFull{false};
};

} // namespace dlt

using DltCppContextClass = logging::dlt::DltCppContextClass;

} // namespace logging
