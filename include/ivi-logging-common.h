#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <string>

#if __cplusplus < 201103L
#error ivi-logging requires C++11
#endif

namespace logging {

#define LOGGING_WARNING_OUTPUT_PREFIX "Logging: "

enum class LogLevel : uint8_t { None, Fatal, Error, Warning, Info, Debug, Verbose, All, Invalid };

struct AppLogContext;

extern AppLogContext* s_pAppLogContext;

std::string getProcessName(pid_t pid);

class ThreadInformation {

  public:
    ThreadInformation() {
        m_id = sNextID++;
    }

    static bool isMultithreadedApp() {
        return (sNextID != 1);
    }

    int getID() const {
        return m_id;
    }

    char const* getName() const;

  private:
    mutable std::string m_name;
    int m_id;
    static std::atomic_int sNextID;
};

ThreadInformation& getThreadInformation();

struct AppLogContext {
    AppLogContext(char const* id, char const* description) : m_id(id), m_description(description) {
        if (s_pAppLogContext != nullptr) {
            fprintf(stderr, LOGGING_WARNING_OUTPUT_PREFIX
                    "An ID has already been defined for your application. Please ensure LOG_DEFINE_APP_IDS macro is called before any log is produced.\n");
        }
        s_pAppLogContext = this;
    }
    std::string m_id;
    std::string m_description;
};

void setDefaultAPPIDSIfNeeded();

/**
 * A logging context
 */
class LogContextCommon {

  public:
    LogContextCommon(std::string const& id, std::string const& contextDescription) : m_id(id), m_description(contextDescription) {
        if (id.length() > 4)
            fprintf(stderr, LOGGING_WARNING_OUTPUT_PREFIX "Log IDs should not be longer than 4 characters to be compatible with the DLT : %s\n", id.c_str());
    }

    char const* getDescription() const {
        return m_description.c_str();
    }

    char const* getID() const {
        return m_id.c_str();
    }

  private:
    const std::string m_id;
    const std::string m_description;
};

class LogContextBase {

  public:
    bool isSourceCodeLocationInfoEnabled() const {
        return m_enableSourceCodeLocationInfo;
    }

    bool isThreadInfoEnabled() const {
        return m_enableThreadInfo;
    }

    void registerContext();

  private:
    static bool m_enableSourceCodeLocationInfo;
    static bool m_enableThreadInfo;
    static bool s_initialized;
};

class LogInfo {

  public:
    LogInfo() {
    }

    LogInfo(LogLevel level, char const* fileName, int lineNumber, char const* prettyFunction) {
        m_level = level;
        m_longFileName = fileName;
        m_lineNumber = lineNumber;
        m_prettyFunction = prettyFunction;
    }

    ~LogInfo() {
        if (m_level == LogLevel::Fatal) {
            std::cerr << "Exiting process due to fatal log\n";
            abort();
        }
    }

    LogLevel getLogLevel() const {
        return m_level;
    }

    char const* getFileName() const;

    int getLineNumber() const {
        return m_lineNumber;
    }

    char const* getPrettyFunction() const {
        return m_prettyFunction;
    }

    bool isHexEnabled() const {
        return m_hexEnabled;
    }

    void setHexEnabled(bool enabled) {
        m_hexEnabled = enabled;
    }

  private:
    char const* m_longFileName;
    mutable char const* m_fileName = nullptr;
    char const* m_prettyFunction;
    int m_lineNumber;
    LogLevel m_level;
    bool m_hexEnabled{false};
};

class LogData {};

std::string getStackTrace(size_t max_frames = 63);

} // namespace logging
