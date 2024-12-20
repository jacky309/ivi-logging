#pragma once

#include "ivi-logging-common.h"
#include "ivi-logging-utils.h"
#include <string_view>

namespace logging {

class StreamLogData;
class ConsoleLogData;

static constexpr char const ANSI_COLOR_OFF[] = "\x1b[0m";
static constexpr char const ANSI_COLOR_BLACK[] = "\x1b[30m";
static constexpr char const ANSI_COLOR_RED[] = "\x1b[31m";
static constexpr char const ANSI_COLOR_GREEN[] = "\x1b[32m";
static constexpr char const ANSI_COLOR_YELLOW[] = "\x1b[33m";
static constexpr char const ANSI_COLOR_BLUE[] = "\x1b[34m";
static constexpr char const ANSI_COLOR_MAGENTA[] = "\x1b[35m";
static constexpr char const ANSI_COLOR_CYAN[] = "\x1b[36m";
static constexpr char const ANSI_COLOR_GRAY[] = "\x1b[37m";
static constexpr char const ANSI_COLOR_BRIGHT[] = "\x1b[1m";
static constexpr char const ANSI_COLOR_DIM[] = "\x1b[2m";
static constexpr char const ANSI_BLINK[] = "\x1b[5m";
static constexpr char const ANSI_RESET_BRIGHT[] = "\x1b[0m";

class StreamLogContextAbstract : public LogContextBase {
  public:
    StreamLogContextAbstract() {
    }

    virtual ~StreamLogContextAbstract() {
    }

    void setParentContext(LogContextCommon& context) {
        m_context = &context;
    }

    char const* getShortID() {
        return m_context->getID();
    }

    virtual FILE* getFile(StreamLogData& data) = 0;

    void setLogLevel(LogLevel level) {
        m_level = level;
    }

    LogLevel getLogLevel() const {
        return m_level;
    }

    virtual bool isEnabled(LogLevel level) const {
        return (level <= getLogLevel());
    }

    void write(char const* s, StreamLogData& data);

    static unsigned int getConsoleWidth();

  private:
    LogContextCommon* m_context = nullptr;
    LogLevel m_level = LogLevel::Debug;
};

/**
 * Context for logging to the console
 */
class ConsoleLogContext : public StreamLogContextAbstract {

  public:
    typedef ConsoleLogData LogDataType;

    static int const DEFAULT_WIDTH = 150;

    ConsoleLogContext();

    bool isEnabled(LogLevel level) const override {
        return (StreamLogContextAbstract::isEnabled(level) && (level <= s_defaultLogLevel));
    }

    /**
     * Sets the global log level for the logging to the console
     * By setting that log level to "None", you ensure that no log will be sent to the console
     * By setting that log level to "All" (default value), you disable the effect of the global filtering, which means
     * that only the log levels of the individual contexts will be taken into account.
     */
    static void setGlobalLogLevel(LogLevel level) {
        s_defaultLogLevel = level;
    }

    FILE* getFile(StreamLogData& data) override;

    bool isColorsEnabled() {
        return m_colorSupport;
    }

  private:
    static LogLevel s_defaultLogLevel;
    static bool s_envVarCheckDone;
    bool m_colorSupport;
};

void getCurrentTime(unsigned int& seconds, unsigned int& milliseconds);

class StreamLogData {

  public:
    static constexpr char const* DEFAULT_PREFIX = "%05d.%03d | %4.4s [%s] "; // with timestamp

    static constexpr char const* DEFAULT_SUFFIX_WITH_FILE_LOCATION = " [ %.2i | %s / %s - %d ]";
    static constexpr char const* DEFAULT_SUFFIX_WITH_SHORT_FILE_LOCATION_WITHOUT_FUNCTION = " | %s%.0s:%d ";
    static constexpr char const* DEFAULT_THREAD_NAME_SUFFIX = "| %.2i / %s ";
    static constexpr char const* DEFAULT_SUFFIX_WITH_SHORT_FILE_LOCATION_WITHOUT_FUNCTION_WITH_THREAD_NAME = " [ %s%.0s - %d | %.2i-%.16s ]";
    static constexpr char const* DEFAULT_SUFFIX_WITHOUT_FILE_LOCATION = "";

    typedef StreamLogContextAbstract ContextType;

    virtual ~StreamLogData() {
    }

    void flushLog() {
        if (isEnabled()) {
            writeSuffix();

            // add terminal null character
            m_content.resize(m_content.size() + 1);
            m_content[m_content.size() - 1] = 0;
            m_context->write(m_content.getData(), *this);
        }
    }

    void setPrefixFormat(char const* format) {
        m_prefixFormat = format;
    }

    void setSuffixFormat(char const* format) {
        m_suffixFormat = format;
    }

    void init(StreamLogContextAbstract& aContext, LogInfo const& data) {
        m_context = &aContext;
        m_data = &data;

        if (isEnabled())
            writePrefix();
    }

    //   const logging::LogData *m_logData;

    virtual void writePrefix() {
        unsigned int seconds, milliseconds;
        getCurrentTime(seconds, milliseconds);
        writeFormatted(m_prefixFormat, seconds, milliseconds, getContext()->getShortID(), getLogLevelString(getData().getLogLevel()));
    }

    virtual void writeSuffix() {
        writeFormatted(getSuffix().getData());
    }

    virtual ByteArray getSuffix() {
        ByteArray array;

        // we ignore the env variable since we always want source code information in the console
        writeFormatted(array, m_suffixFormat, getData().getFileName(), getData().getPrettyFunction(), getData().getLineNumber());

        if (m_context->isThreadInfoEnabled()) {
            writeFormatted(array, m_suffixThreadNameFormat, getThreadInformation().getID(), getThreadInformation().getName());
        }
        return array;
    }

    ContextType* getContext() {
        return m_context;
    }

    static char const* getLogLevelString(LogLevel logLevel) {
        char const* v = "";
        switch (logLevel) {
            case LogLevel::Debug:
                v = " Debug ";
                break;
            case LogLevel::Info:
                v = " Info  ";
                break;
            case LogLevel::Warning:
                v = "Warning";
                break;
            case LogLevel::Error:
                v = " Error ";
                break;
            case LogLevel::Fatal:
                v = " Fatal ";
                break;
            case LogLevel::Verbose:
                v = "Verbose";
                break;
            default:
                v = "Invalid";
                break;
        }
        return v;
    }

    bool isEnabled() {
        return (m_context->isEnabled(getData().getLogLevel()));
    }

    LogInfo const& getData() const {
        return *m_data;
    }

    template <typename... Args>
    StreamLogData& writeFormatted(char const* format, Args... args) {
        if (isEnabled()) {
            writeFormatted(m_content, format, args...);
        }
        return *this;
    }

    StreamLogData& writeInt(int value) {
        return writeFormatted(isHexEnabled() ? "%X" : "%d", value);
    }

    StreamLogData& writeInt(unsigned int value) {
        return writeFormatted(isHexEnabled() ? "%X" : "%u", value);
    }

    template <typename... Args>
    void writeFormatted(ByteArray& byteArray, char const* format, Args... args) const {

#pragma GCC diagnostic push
        // Make sure GCC does not complain about not being able to check the format string since it is no literal string
#pragma GCC diagnostic ignored "-Wformat-security"
        int size = snprintf(nullptr, 0, format, args...) + 1; // +1 since the snprintf returns the number of characters excluding the null termination
        size_t startOfStringIndex = byteArray.size();
        byteArray.resize(byteArray.size() + size);
        char* p = byteArray.getData() + startOfStringIndex;
        snprintf(p, size, format, args...);

        // remove terminal null character
        byteArray.resize(byteArray.size() - 1);
#pragma GCC diagnostic pop
    }

    bool isHexEnabled() const {
        return m_data->isHexEnabled();
    }

    void write(std::string_view v) {
        writeFormatted("%.*s", static_cast<int>(v.length()), v.data());
    }

    void write(bool v) {
        writeFormatted("%s", v ? "true" : "false");
    }

    void write(char v) {
        writeInt(v);
    }

    void write(unsigned char v) {
        writeInt(v);
    }

    void write(signed short v) {
        writeInt(v);
    }

    void write(unsigned short v) {
        writeInt(v);
    }

    void write(signed int v) {
        writeInt(v);
    }

    void write(unsigned int v) {
        writeInt(v);
    }

    void write(long long v) {
        writeFormatted(isHexEnabled() ? "%llX" : "%lld", v);
    }

    void write(unsigned long long v) {
        writeFormatted(isHexEnabled() ? "%llX" : "%llu", v);
    }

    void write(signed long v) {
        writeFormatted(isHexEnabled() ? "%lX" : "%ld", v);
    }

    void write(unsigned long v) {
        writeFormatted(isHexEnabled() ? "%lX" : "%lu", v);
    }

    template <typename Type>
    void write(Type const* v) {
        writeFormatted("%s", pointerToString(v).c_str());
    }

    void write(char const* v) {
        writeFormatted("%s", v ? v : "null");
    }

    void write(float v) {
        writeFormatted("%f", v);
    }

    void write(std::string const& s) {
        writeFormatted("%s", s.c_str());
    }

    void write(double v) {
        writeFormatted("%f", v);
    }

  protected:
    ContextType* m_context{};
    LogInfo const* m_data{};
    ByteArray m_content;

    char const* m_prefixFormat = DEFAULT_PREFIX;
    //	const char* m_suffixFormat = DEFAULT_SUFFIX_WITHOUT_FILE_LOCATION;
    //	const char* m_suffixFormat = DEFAULT_SUFFIX_WITH_SHORT_FILE_LOCATION_WITHOUT_FUNCTION_WITH_THREAD_NAME;
    char const* m_suffixFormat = DEFAULT_SUFFIX_WITH_SHORT_FILE_LOCATION_WITHOUT_FUNCTION;
    char const* m_suffixThreadNameFormat = DEFAULT_THREAD_NAME_SUFFIX;
};

inline FILE* ConsoleLogContext::getFile(StreamLogData& data) {
    if (data.getData().getLogLevel() == LogLevel::Error)
        return stderr;
    else
        return stdout;
}

class ConsoleLogData : public StreamLogData {

  public:
    typedef ConsoleLogContext ContextType;

    virtual ~ConsoleLogData() {
        flushLog();
    }

    void init(ContextType& aContext, LogInfo const& data) {
        m_context = &aContext;
        StreamLogData::init(aContext, data);
    }

    enum class Command { RESET = 0, BRIGHT = 1, DIM = 2, UNDERLINE = 3, BLINK = 4, REVERSE = 7, HIDDEN = 8 };

    enum class Color { BLACK = 0, RED = 1, GREEN = 2, YELLOW = 3, BLUE = 4, MAGENTA = 5, CYAN = 6, WHITE = 7 };

    virtual void writePrefix() override {
        writeHeaderColor();
        StreamLogData::writePrefix();
        resetColor();
    }

    virtual void writeSuffix() override {

        writeFooterColor();

        ByteArray suffixArray = getSuffix();

        writeFormatted(suffixArray, "\n");

        int width = m_context->getConsoleWidth();

        // If no width is available, use default width
        if (width == 0)
            width = ContextType::DEFAULT_WIDTH;

        width -= m_content.size() + suffixArray.size() - m_invisibleCharacterCount;

        // If the output line is longer that the console width, we print our suffix on the next line
        if (width < 0) {
            writeFormatted("\n");
            width = m_context->getConsoleWidth() - static_cast<int>(suffixArray.size());
        }

        for (int i = 0; i < width; i++) {
            writeFormatted(" ");
        }

        writeFormatted("%s", suffixArray.getData());

        resetColor();
    }

    void writeHeaderColor() {
        if (!m_context->isColorsEnabled())
            return;

        std::string s = ANSI_COLOR_OFF;
        switch (getData().getLogLevel()) {
            case LogLevel::Warning:
                s = ANSI_COLOR_MAGENTA;
                break;
            case LogLevel::Error:
            case LogLevel::Fatal:
                s = ANSI_COLOR_RED;
                s += ANSI_COLOR_BRIGHT;
                break;
            case LogLevel::Verbose:
                s = ANSI_COLOR_GREEN;
                break;
            default:
                s = ANSI_COLOR_OFF;
                break;
        }
        write(s);
        m_invisibleCharacterCount += s.length();
    }

    void writeFooterColor() {
        writeHeaderColor();
    }

    void resetColor() {
        if (!m_context->isColorsEnabled())
            return;

        std::string s = ANSI_COLOR_OFF;
        s += ANSI_RESET_BRIGHT;
        write(s);
        m_invisibleCharacterCount += s.length();
    }

    int m_invisibleCharacterCount = 0;
    ContextType* m_context;
};

} // namespace logging
