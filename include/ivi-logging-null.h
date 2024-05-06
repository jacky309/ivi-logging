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

    template<typename ... Args>
    void writeFormatted(const char*, Args ...) {
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

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] bool v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] const char* v) {
    return data;
}

template<size_t N>
inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] const char (&v)[N]) {
   return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] const std::string& v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] float v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] double v) {
    return data;
}

template<typename T, typename = typename std::enable_if<std::is_integral_v<T>>::type>
inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] const T& v) {
    return data;
}

}
