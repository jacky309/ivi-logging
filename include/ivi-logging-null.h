#pragma once

#include "ivi-logging-common.h"
#include <type_traits>

namespace logging {

class NullLogContext;

/**
 * This class provides the same interface as other LogData classes, but actually does nothing with the objects which are streamed to it.
 * It can be useful to disable some logging.
 */
class NullLogData : public LogData {

  public:
    void init([[maybe_unused]] NullLogContext& context, [[maybe_unused]] LogInfo& data) {
    }

    bool isEnabled() const {
        return false;
    }

    template <typename... Args>
    void writeFormatted(char const*, Args...) {
    }

    template <typename Type>
    void write(Type const&) {
    }
};

class NullLogContext {

  public:
    typedef NullLogData LogDataType;

    void setParentContext([[maybe_unused]] LogContextCommon& context) {
    }

    bool isEnabled([[maybe_unused]] LogLevel logLevel) {
        return false;
    }

    void registerContext() {
    }
};

} // namespace logging
