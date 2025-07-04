#pragma once

#include "dlt_user.h"
#include "ivi-logging-common.h"
#include <array>
#include <cstring>
#include <sys/time.h>
#include <sys/uio.h>
#include <thread>

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
} __attribute__((packed)) DltUserControlMsgLogLevel;

typedef struct {
    int8_t log_state; /**< the state to be used for logging state: 0 = off, 1 = external client connected */
} __attribute__((packed)) DltUserControlMsgLogState;

typedef struct {
    char pattern[DLT_ID_SIZE]{'D', 'U', 'H', 1}; /**< This pattern should be DUH0x01 */
    MessageType message;                         /**< messsage info */
} __attribute__((packed)) DltUserHeader;

typedef struct {
    char apid[DLT_ID_SIZE]{};    /**< application id */
    pid_t pid;                   /**< process id of user application */
    uint32_t description_length; /**< length of description */
} __attribute__((packed)) DltUserControlMsgRegisterApplication;

typedef struct {
    char apid[DLT_ID_SIZE]{};    /**< application id */
    char ctid[DLT_ID_SIZE]{};    /**< context id */
    int32_t log_level_pos{};     /**< offset in management structure on user-application side */
    int8_t log_level{};          /**< log level */
    int8_t trace_status{};       /**< trace status */
    pid_t pid;                   /**< process id of user application */
    uint32_t description_length; /**< length of description */
} __attribute__((packed)) DltUserControlMsgRegisterContext;

typedef struct {
    char pattern[DLT_ID_SIZE]{'D', 'L', 'T', 1}; /**< This pattern should be DLT0x01 */
    uint32_t seconds;                            /**< seconds since 1.1.1970 */
    int32_t microseconds;                        /**< Microseconds */
    char ecu[DLT_ID_SIZE]{'E', 'C', 'U', '1'};   /**< The ECU id is added, if it is not already in the DLT message itself */
} __attribute__((packed)) DltStorageHeader;

/**
 * The structure of the DLT standard header. This header is used in each DLT message.
 */
typedef struct {
    uint8_t htyp{DLT_HTYP_PROTOCOL_VERSION1 | DLT_HTYP_WEID | DLT_HTYP_WTMS | DLT_HTYP_WSID |
                 DLT_HTYP_UEH}; /**< This parameter contains several informations, see definitions below */
    uint8_t mcnt;               /**< The message counter is increased with each sent DLT message */
    uint16_t len;               /**< Length of the complete message, without storage header */
} __attribute__((packed)) DltStandardHeader;

/**
 * The structure of the DLT extra header parameters. Each parameter is sent only if enabled in htyp.
 */
typedef struct {
    char ecu[DLT_ID_SIZE]{'E', 'C', 'U', '1'}; /**< ECU id */
    uint32_t seid{};                           /**< Session number */
    uint32_t tmsp;                             /**< Timestamp since system start in 0.1 milliseconds */
} __attribute__((packed)) DltStandardHeaderExtra;

/**
 * The structure of the DLT extended header. This header is only sent if enabled in htyp parameter.
 */
typedef struct {
    uint8_t msin{DLT_MSIN_VERB | (DLT_TYPE_LOG << DLT_MSIN_MSTP_SHIFT)}; /**< messsage info */
    uint8_t noar;                                                        /**< number of arguments */
    char apid[DLT_ID_SIZE];                                              /**< application id */
    char ctid[DLT_ID_SIZE];                                              /**< context id */
} __attribute__((packed)) DltExtendedHeader;

class DltCppLogData;

class DltCppContextClass;

class DaemonConnection {
  public:
    static DaemonConnection& getInstance();

    ~DaemonConnection();

    void initDaemonConnection();

    template <typename... Types>
    void send(Types const&... values);

    void handleIncomingMessage();

    int32_t registerContext(DltCppContextClass& context);

    void applyLogLevel(DltUserControlMsgLogLevel const& message);

    void sendLog(DltCppLogData& data);

    void sendContextRegistration(DltCppContextClass& context);

  private:
    bool isDaemonConnected() const {
        return (m_daemonFileDescriptor != disconnectedFromDaemonFd);
    }

    void init();

    std::thread readerThread;

    static constexpr int disconnectedFromDaemonFd = -1;

    int m_daemonFileDescriptor{disconnectedFromDaemonFd};
    int m_appFileDescriptor;
    bool m_initialized{false};
    bool m_stopRequested{false};
};

class DltCppContextClass : public LogContextBase {

  public:
    using LogDataType = DltCppLogData;

    DltCppContextClass() {
    }

    ~DltCppContextClass() {
    }

    void setParentContext(LogContextCommon& context) {
        m_context = &context;
    }

    bool isEnabled(LogLevel logLevel) const {
        auto const dltLogLevel = getDLTLogLevel(logLevel);
        return dltLogLevel <= m_activeLogLevel;
    }

    auto messageCounter() {
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
        DaemonConnection::getInstance().registerContext(*this);
    }

    void setActiveLogLevel(DltLogLevelType activeLogLevel);

  private:
    LogContextCommon* m_context = nullptr;
    DltExtendedHeader extendedHeader{};
    std::atomic<uint8_t> m_messageCount{0};

    int32_t log_level_pos{-1};

    DltLogLevelType m_activeLogLevel{DltLogLevelType::DLT_LOG_VERBOSE};

    friend class DltCppLogData;
    friend class DaemonConnection;
};

class DltCppLogData : public ::logging::LogData {

  public:
    using ContextType = DltCppContextClass;
    using DltTypeInfo = uint32_t;
    using DltStringLengthType = uint16_t;

    DltCppLogData() {
    }

    void init(DltCppContextClass& context, LogInfo const& data) {
        m_data = &data;
        m_context = &context;
        m_dltLogLevel = m_context->getDLTLogLevel(getData().getLogLevel());
        m_enabled = context.isEnabled(getData().getLogLevel());

        m_messageCount = context.messageCounter();

#ifndef NDEBUG
        m_content.fill(-1);
#endif
    }

    virtual ~DltCppLogData();

    LogInfo const& getData() const {
        return *m_data;
    }

    bool isEnabled() const {
        return m_enabled;
    }

    bool isHexEnabled() const {
        return m_data->isHexEnabled();
    }

    size_t getAvailableSpace(size_t infoSize) const {
        if (not m_isFull and (maxContentSize - m_contentSize >= infoSize)) {
            return maxContentSize - m_contentSize - infoSize;
        } else {
            return 0;
        }
    }

    static constexpr size_t messageTooLargeStringLength = 12;

    struct __attribute__((packed)) MessageTooLargeData {
        DltTypeInfo const info{DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8};
        DltStringLengthType length = messageTooLargeStringLength;
        char const message[messageTooLargeStringLength]{'.', '.', 't', 'r', 'u', 'n', 'c', 'a', 't', 'e', 'd', 0};
    };

    void addMessageTooLargeIndication();

    bool checkOverflow(size_t additionalSize) {
        if (not m_isFull and getAvailableSpace(additionalSize) == 0) {
            addMessageTooLargeIndication();
        }
        return not m_isFull;
    }

    template <typename Type>
    Type* reserve() {
        auto v = reinterpret_cast<Type*>(m_content.data() + m_contentSize);
        m_contentSize += sizeof(Type);
        return v;
    }

    template <typename... Args>
    void writeFormatted(char const* format, Args... args) {

        auto const spaceForStringContent = getAvailableSpace(sizeof(DltStringLengthType) + sizeof(DltTypeInfo) + 1);

        bool overflow = true;

        if (spaceForStringContent > 0) {
            writeType(DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8);

            auto size = reserve<DltStringLengthType>();

#pragma GCC diagnostic push
            // Make sure GCC does not complain about not being able to check the format string since it is no literal string
#pragma GCC diagnostic ignored "-Wformat-security"
            auto const stringSize = static_cast<size_t>(snprintf(m_content.data() + m_contentSize, spaceForStringContent, format, args...));
#pragma GCC diagnostic pop

            // snprintf return value does not include the size for the null termination
            auto const stringSizeInBuffer = std::min(stringSize + 1, spaceForStringContent);

            *size = stringSizeInBuffer;
            m_contentSize += stringSizeInBuffer;
            overflow = (spaceForStringContent < stringSize);
        }

        if (overflow) {
            // No space left for the whole string
            addMessageTooLargeIndication();
        }
    }

    void write(char const* v) {
        write(v, strlen(v));
    }

    void writeBuffer(void const* v, size_t size) {
        if (not m_isFull) {
            memcpy(m_content.data() + m_contentSize, v, size);
            m_contentSize += size;
        }
    }

    void write(char const* v, size_t size) {
        auto const spaceForStringContent = std::min(size, getAvailableSpace(sizeof(DltStringLengthType) + sizeof(DltTypeInfo) + 1));

        if (spaceForStringContent > 0) {
            writeType(DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8);
            DltStringLengthType const sizeAsUint16 = static_cast<DltStringLengthType>(spaceForStringContent) + 1;
            writeBuffer(&sizeAsUint16, sizeof(sizeAsUint16));
            writeBuffer(v, spaceForStringContent);
            static constexpr char nullTermination = 0;
            writeBuffer(&nullTermination, sizeof(nullTermination));
        }

        if (spaceForStringContent < size) {
            // Nos space left for the whole string
            addMessageTooLargeIndication();
        }
    }

    void write(std::string_view v) {
        write(v.data(), v.size());
    }

    void write(std::string const& v) {
        write(v.c_str(), v.size());
    }

    void write(bool v) {
        writeWithTypeInfo(DLT_TYPE_INFO_BOOL, v);
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
        writeWithTypeInfo(DLT_TYPE_INFO_SINT | DLT_TYLE_32BIT, v);
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
    void writeWithTypeInfo(DltTypeInfo type, Type const& value) {
        if (checkOverflow(sizeof(value) + sizeof(DltTypeInfo))) {
            writeType(type);
            writeBuffer(&value, sizeof(value));
        }
    }

    void writeType(DltTypeInfo type) {
        writeBuffer(&type, sizeof(type));
        m_argsCount++;
    }

  private:
    static constexpr size_t maxContentSize = 2048 - sizeof(MessageTooLargeData);
    std::array<char, maxContentSize + sizeof(MessageTooLargeData)> m_content;

    size_t m_contentSize{0};

    DltCppContextClass* m_context{};
    LogInfo const* m_data{};

    DltLogLevelType m_dltLogLevel;
    bool m_enabled{false};
    uint8_t m_argsCount = 0;

    bool m_isFull{false};
    uint8_t m_messageCount;

    static constexpr DltUserHeader sendLogUserHeader{.message = MessageType::SendLog};

    friend class DaemonConnection;
};

} // namespace dlt

using DltCppContextClass = logging::dlt::DltCppContextClass;

} // namespace logging
