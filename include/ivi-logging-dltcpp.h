#pragma once

#include "dlt_user.h"
#include "ivi-logging-common.h"
#include <array>
#include <cstring>

namespace logging {

namespace dlt {

enum class MessageType : uint32_t { SendLog = 1, RegisterApp = 2, RegisterContext = 4, DLT_USER_MESSAGE_LOG_STATE = 12, DLT_USER_MESSAGE_LOG_LEVEL = 6 };

typedef struct {
    char pattern[DLT_ID_SIZE]{'D', 'U', 'H', 1}; /**< This pattern should be DUH0x01 */
    MessageType message;                         /**< messsage info */
} __attribute__((packed)) DltUserHeader;

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

    void registerContext();

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

    void init(DltCppContextClass& context, LogInfo const& data);

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
    static constexpr size_t maxMessageSize = 16384; // Maximum size of a log
    static constexpr size_t maxContentSize = maxMessageSize - sizeof(MessageTooLargeData);
    std::array<char, maxMessageSize> m_content;

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
