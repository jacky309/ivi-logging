#include "ivi-logging-console.h"
#include "stdint.h"
#include "stdio.h"
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <mutex>
#include <pthread.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

namespace logging {

AppLogContext* s_pAppLogContext = nullptr;

LogLevel ConsoleLogContext::s_defaultLogLevel = LogLevel::Debug;
bool ConsoleLogContext::s_envVarCheckDone = false;

std::mutex streamLogContextAbstractOutputMutex;

std::atomic_int ThreadInformation::sNextID = ATOMIC_VAR_INIT(0);

#if __GNUC_PREREQ(4, 8)
// Note the the initialization of a "thread_local" variable occurs during the first access to it, which is not what we would like, since we
// would like our thread ID to be set as soon as a new thread is spawned. TODO : check how to achieve that.
static thread_local ThreadInformation threadID__;
ThreadInformation& getThreadInformation() {
    return threadID__;
}
#else

// Alternative implementation not using "thread_local" keyword for GCC < 4.8

static __thread ThreadInformation* threadID__ = nullptr;

ThreadInformation& getThreadInformation() {
    if (threadID__ == nullptr)
        threadID__ = new ThreadInformation();
    return *threadID__;
}

#endif

void setDefaultAPPIDSIfNeeded() {
    if (s_pAppLogContext == nullptr) {
        //		fprintf(
        //			stderr,
        //			LOGGING_WARNING_OUTPUT_PREFIX
        //			"Your application should define its ID using the LOG_DEFINE_APP_IDS macro\n");
        pid_t pid = getpid();
        char pidAsHex[5];
        snprintf(pidAsHex, sizeof(pidAsHex), "%X", pid);
        //		char pidAsDecimal[6];
        //		snprintf(pidAsDecimal, sizeof(pidAsDecimal), "%i", pid);
        //		std::string processName = "PID:";
        //		processName += pidAsDecimal;
        //		processName += " / ";
        //		std::string processName = getProcessName(pid);
        static AppLogContext defaultAppLogContext(pidAsHex, getProcessName(pid).c_str());
        s_pAppLogContext = &defaultAppLogContext;
    }
}

std::string byteArrayToString(void const* buffer, size_t length) {
    static char hexcode[] = "0123456789ABCDEF";
    static char textBuffer[1024];

    size_t dest = 0;

    if (length + 1 > sizeof(textBuffer) / 3)
        length = sizeof(textBuffer) / 3;

    unsigned char const* bufferAsChar = static_cast<unsigned char const*>(buffer);

    for (size_t byteIndex = 0; byteIndex < length; byteIndex++) {
        textBuffer[dest++] = hexcode[bufferAsChar[byteIndex] >> 4];
        textBuffer[dest++] = hexcode[bufferAsChar[byteIndex] & 0xF];
        textBuffer[dest++] = ' ';
    }

    textBuffer[dest] = 0;

    return textBuffer;
}

unsigned int StreamLogContextAbstract::getConsoleWidth() {
    struct ::winsize ws;
    if (::ioctl(0, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_col;
    } else
        return 0;
}

std::string getProcessName(pid_t pid) {

    char processName[1024] = "";

    DIR* dir = opendir("/proc");
    if (dir != nullptr) {
        char path[128];
        snprintf(path, sizeof(path), "/proc/%i/cmdline", pid);
        FILE* file = fopen(path, "r");

        if (file != nullptr) {
            size_t n = fread(processName, 1, sizeof(processName) - 1, file);
            if (n > 0)
                processName[n - 1] = 0;
            fclose(file);
        }
    }

    if (dir != nullptr)
        closedir(dir);

    if (strlen(processName) == 0)
        snprintf(processName, sizeof(processName), "Unknown process with PID %i", pid);

    return processName;
}

char const* ThreadInformation::getName() const {
    std::array<char, 64> buffer;
    auto ret = pthread_getname_np(pthread_self(), buffer.data(), std::tuple_size_v<decltype(buffer)>);
    if (ret != 0)
        buffer[0] = 0;
    m_name = buffer.data();
    return m_name.c_str();
}

static bool readEnvVarAsBool(char const* varName, bool defaultValue = false) {
    auto value = getenv(varName);
    if (value == nullptr)
        return defaultValue;
    return !strcmp(value, "1");
}

void LogContextBase::registerContext() {
    if (!s_initialized) {
        m_enableSourceCodeLocationInfo = readEnvVarAsBool("LOGGING_ENABLE_SOURCE_CODE_INFORMATION");
        m_enableThreadInfo = readEnvVarAsBool("LOGGING_ENABLE_THREAD_INFORMATION");
        s_initialized = true;
    }
}

bool LogContextBase::m_enableSourceCodeLocationInfo;
bool LogContextBase::m_enableThreadInfo;
bool LogContextBase::s_initialized;

ConsoleLogContext::ConsoleLogContext() {
    m_colorSupport = (getConsoleWidth() != 0);
    if (!s_envVarCheckDone) {
        if (not readEnvVarAsBool("LOGGING_ENABLE_CONSOLE", true)) {
            s_defaultLogLevel = LogLevel::None;
        } else {
            char const* logLevelEnv = getenv("LOGGING_CONSOLE_LOGLEVEL");
            if (logLevelEnv != nullptr) {
                std::string levelString = logLevelEnv;
                for (auto& c : levelString) {
                    c = std::tolower(c);
                }
                static constexpr std::array<std::pair<LogLevel, char const*>, 7> logLevelStrings{{
                    {LogLevel::None, "none"},
                    {LogLevel::Fatal, "fatal"},
                    {LogLevel::Error, "error"},
                    {LogLevel::Warning, "warning"},
                    {LogLevel::Info, "info"},
                    {LogLevel::Debug, "debug"},
                    {LogLevel::Verbose, "verbose"},
                }};

                for (auto const& entry : logLevelStrings) {
                    if (levelString == entry.second) {
                        s_defaultLogLevel = entry.first;
                    }
                }
            }
        }
        s_envVarCheckDone = true;
    }
    setLogLevel(s_defaultLogLevel);
}

char const* LogInfo::getFileName() const {
    if (m_fileName == nullptr) {
        size_t shortNamePosition = strlen(m_longFileName);
        while ((shortNamePosition > 0) && (m_longFileName[shortNamePosition - 1] != '/'))
            shortNamePosition--;
        m_fileName = m_longFileName + shortNamePosition;
    }
    return m_fileName;
}

void StreamLogContextAbstract::write(char const* s, StreamLogData& data) {
    std::lock_guard<std::mutex> lock(streamLogContextAbstractOutputMutex);
    auto file = getFile(data);
    if (file) {
        fprintf(file, "%s", s);
        fflush(file);
    }
}

static auto loggingStartTime{std::chrono::system_clock::now()};

void getCurrentTime(unsigned int& seconds, unsigned int& milliseconds) {
    auto elapsedTime = std::chrono::system_clock::now() - loggingStartTime;
    seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();
    milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime - std::chrono::seconds{seconds}).count();
}

} // namespace logging
