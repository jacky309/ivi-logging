#pragma once

#include <functional>
#include <sstream>
#include <string>

#include "ivi-logging-common.h"

namespace logging {

template <size_t I = 0, typename Func, typename... TupleTypes, typename... CallArgumentTypes>
typename std::enable_if<I == sizeof...(TupleTypes)>::type for_each_in_tuple_([[maybe_unused]] std::tuple<TupleTypes...>& tpl, [[maybe_unused]] Func func,
                                                                             [[maybe_unused]] CallArgumentTypes&... args) {
}

template <size_t I = 0, typename Func, typename... TupleTypes, typename... CallArgumentTypes>
    typename std::enable_if < I<sizeof...(TupleTypes)>::type for_each_in_tuple_(std::tuple<TupleTypes...>& tpl, Func func, CallArgumentTypes&... args) {
    func(std::get<I>(tpl), args...);
    for_each_in_tuple_<I + 1>(tpl, func, args...);
}

inline std::string pointerToString(void const* p) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "0x%zX", (size_t)p);
    return buffer;
}

class StringBuilder {
  public:
    template <typename Type>
    StringBuilder& operator<<(Type const& v) {
        m_stream << v;
        return *this;
    }

    StringBuilder& operator<<(uint8_t const& v) {
        m_stream << (int)v;
        return *this;
    }

    StringBuilder& operator<<(int8_t const& v) {
        m_stream << (int)v;
        return *this;
    }

    operator std::string() {
        return m_stream.str();
    }

    std::string str() const {
        return m_stream.str();
    }

  private:
    std::stringstream m_stream;
};

#define log_with_context(_context_, severity)                                                                                                                  \
    for (auto dummy = &(_context_); (dummy != nullptr) && dummy->isEnabled(severity); dummy = nullptr)                                                         \
    (_context_).createLog(severity, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#ifndef log_error

#define log_with_severity(severity) log_with_context(getDefaultContext(), severity)

/**
 * Generate a log with "fatal" severity
 */
#define log_fatal() log_fatal_with_context(getDefaultContext())

/**
 * Generate a log with "error" severity
 */
#define log_error() log_error_with_context(getDefaultContext())

/**
 * Generate a log with "verbose" severity
 */
#define log_verbose() log_verbose_with_context(getDefaultContext())

/**
 * Generate a log with "info" severity
 */
#define log_info() log_info_with_context(getDefaultContext())

/**
 * Generate a log with "warning" severity
 */
#define log_warn() log_with_context(getDefaultContext(), logging::LogLevel::Warning)
#define log_warning() log_warn()

/**
 * Generate a log with "debug" severity
 */
#define log_debug() log_with_context(getDefaultContext(), logging::LogLevel::Debug)

/**
 * Generate a log with "fatal" severity
 */
#define log_fatal_with_context(context) log_with_context(context, logging::LogLevel::Fatal)

/**
 * Generate a log with "error" severity
 */
#define log_error_with_context(context) log_with_context(context, logging::LogLevel::Error)

/**
 * Generate a log with "verbose" severity
 */
#define log_verbose_with_context(context) log_with_context(context, logging::LogLevel::Verbose)

/**
 * Generate a log with "info" severity
 */
#define log_info_with_context(context) log_with_context(context, logging::LogLevel::Info)

/**
 * Generate a log with "warning" severity
 */
#define log_warn_with_context(context) log_with_context(context, logging::LogLevel::Warning)

/**
 * Generate a log with "debug" severity
 */
#define log_debug_with_context(context) log_with_context(context, logging::LogLevel::Debug)

/**
 * Defines the identifiers of an application. This macro should be used at one place in every application.
 */
#define LOG_DEFINE_APP_IDS(appID, appDescription)                                                                                                              \
    logging::AppLogContext s_appLogContext {                                                                                                                   \
        appID, appDescription                                                                                                                                  \
    }

/**
 * Create a LogContext with the given ID (4 characters in case of DLT support) and description
 */
#define LOG_DECLARE_CONTEXT(contextName, contextShortID, contextDescription)                                                                                   \
    LogContext contextName {                                                                                                                                   \
        contextShortID, contextDescription                                                                                                                     \
    }

/**
 * Create a new context and define is as default context for the current scope
 */
#define LOG_DECLARE_DEFAULT_CONTEXT(context, contextID, contextDescription)                                                                                    \
    LOG_DECLARE_CONTEXT(context, contextID, contextDescription);                                                                                               \
    LOG_SET_DEFAULT_CONTEXT(context)

/**
 * Import the given context, which should be exported by another module
 */
#define LOG_IMPORT_CONTEXT(contextName) extern LogContext contextName

/**
 * Set the given context as default for the current scope
 */
#define LOG_SET_DEFAULT_CONTEXT(context)                                                                                                                       \
    static constexpr auto getDefaultContext = []() -> auto& {                                                                                                  \
        return context;                                                                                                                                        \
    }

/**
 * Import the given context and set it as default for the current scope
 */
#define LOG_IMPORT_DEFAULT_CONTEXT(context)                                                                                                                    \
    LOG_IMPORT_CONTEXT(context);                                                                                                                               \
    LOG_SET_DEFAULT_CONTEXT(context)

/**
 * Set the given context as default for the current class
 */
#define LOG_SET_CLASS_CONTEXT(context)                                                                                                                         \
    static constexpr inline auto& getDefaultContext() {                                                                                                        \
        return context;                                                                                                                                        \
    }

/**
 *
 */
#define LOG_DECLARE_DEFAULT_LOCAL_CONTEXT(contextShortID, contextDescription)                                                                                  \
    static constexpr auto getDefaultContext = []() -> auto& {                                                                                                  \
        static LogContext __defaultContext(contextShortID, contextDescription);                                                                                \
        return __defaultContext;                                                                                                                               \
    }

/**
 * Set a new context as default context for a class. To be used inside the class definition.
 */
#define LOG_DECLARE_CLASS_CONTEXT(contextShortID, contextDescription)                                                                                          \
    static auto& getDefaultContext() {                                                                                                                         \
        static LogContext __defaultLogContext(contextShortID, contextDescription);                                                                             \
        return __defaultLogContext;                                                                                                                            \
    }

#endif

template <typename... Types>
struct TypeSet {};

template <class, class>
class LogContextT;

template <template <class...> class ContextTypesClass, class... ContextTypes, template <class...> class ContextDataTypesClass, class... LogDataTypes>
class LogContextT<ContextTypesClass<ContextTypes...>, ContextDataTypesClass<LogDataTypes...>> : public LogContextCommon {

  public:
    /**
     * Use that typedef to extend a LogContext type by adding a backend to it.
     */
    template <typename ExtraContextType, typename ExtraDataType>
    using Extension =
        typename LogContextT<ContextTypesClass<ContextTypes..., ExtraContextType>, ContextDataTypesClass<LogDataTypes..., ExtraDataType>>::LogContextT;

    class LogData : LogInfo {

        template <size_t I = 0, typename... CallArgumentTypes>
        typename std::enable_if<I == sizeof...(ContextTypes)>::type
        for_each_init([[maybe_unused]] std::tuple<LogDataTypes...>& tpl,
                      [[maybe_unused]] LogContextT<ContextTypesClass<ContextTypes...>, ContextDataTypesClass<LogDataTypes...>>& context,
                      [[maybe_unused]] CallArgumentTypes&... args) {
        }

        template <size_t I = 0, typename... CallArgumentTypes>
            typename std::enable_if <
            I<sizeof...(ContextTypes)>::type for_each_init(std::tuple<LogDataTypes...>& tpl,
                                                           LogContextT<ContextTypesClass<ContextTypes...>, ContextDataTypesClass<LogDataTypes...>>& context,
                                                           CallArgumentTypes&... args) {
            std::get<I>(tpl).init(std::get<I>(context.m_contexts), args...);
            for_each_init<I + 1>(tpl, context, args...);
        }

      public:
        LogData(LogContextT<ContextTypesClass<ContextTypes...>, ContextDataTypesClass<LogDataTypes...>>& context, LogLevel level, char const* fileName,
                int lineNumber, char const* prettyFunction)
            : LogInfo(level, fileName, lineNumber, prettyFunction), m_context(context) {
            for_each_init(m_contexts, context, *this);
        }

        ~LogData() {
        }

        LogData& write() {
            return *this;
        }

        template <typename Arg1, typename... Args>
        LogData& write(Arg1 const& firstArg, Args const&... remainingArguments) {
            if (m_context.isEnabled(getLogLevel())) {
                *this << firstArg;
                return write(remainingArguments...);
            }
            return *this;
        }

        template <typename... Args>
        LogData& writeFormatted(char const* format, Args... args) {
            for_each_in_tuple_(m_contexts, [&](auto& log) {
                if (log.isEnabled()) // We need to check each context here to ensure that we don't send data to a disabled context
                    log.writeFormatted(format, args...);
            });
            return *this;
        }

        template <typename Type>
        LogData& operator<<(Type const& v) {
            for_each_in_tuple_(m_contexts, [&](auto& log) {
                if (log.isEnabled()) // We need to check each context here to ensure that we don't send data to a disabled context
                    log << v;
            });
            return *this;
        }

        /**
         * Used to support std::endl, std::ends, etc...
         */
        LogData& operator<<(LogData& (*functionPtr)(LogData&)) {
            functionPtr(*this);
            return *this;
        }

        void setHexEnabled(bool enabled) {
            for_each_in_tuple_(m_contexts, [&](auto& log) {
                log.setHexEnabled(enabled);
            });
        }

      private:
        std::tuple<LogDataTypes...> m_contexts;
        LogContextT<ContextTypesClass<ContextTypes...>, ContextDataTypesClass<LogDataTypes...>>& m_context;
    };

    LogContextT(std::string const& id, std::string const& contextDescription) : LogContextCommon(id, contextDescription) {
        for_each_in_tuple_(m_contexts, [&](auto& log) {
            log.setParentContext(*this);
        });
    }

    LogData createLog(LogLevel level, char const* fileName, int lineNumber, char const* prettyFunction) {
        return LogData(*this, level, fileName, lineNumber, prettyFunction);
    }

    bool isEnabled(LogLevel level) {
        checkContext();
        bool enabled = false;
        for_each_in_tuple_(m_contexts, [&](auto& log) {
            enabled |= log.isEnabled(level);
        });
        return enabled;
    }

    void checkContext() {
        if (!m_bRegistered) {
            setDefaultAPPIDSIfNeeded();
            for_each_in_tuple_(m_contexts, [&](auto& log) {
                log.registerContext();
            });
            m_bRegistered = true;
        }
    }

    std::tuple<ContextTypes...> m_contexts;
    bool m_bRegistered = false;
};

/**
 * Use this context definition to completely disable the logging
 */
typedef LogContextT<TypeSet<>, TypeSet<>> NoLoggingLogContext;

} // namespace logging

#include "ivi-logging-null.h"
#include "ivi-logging-types.h"

#include "ivi-logging-config.h"
