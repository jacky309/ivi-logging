#pragma once

#include "ivi-logging-common.h"

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


// TODO : strangely, it seems like none of the types defined in "stdint.h" is equivalent to "long int" on a 32 bits platform
#if __WORDSIZE == 32
inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] long int v) {
    return data;
}
inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] unsigned long int v) {
    return data;
}
#endif

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] double v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] uint64_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] int64_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] uint32_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] int32_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] uint16_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] int16_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] uint8_t v) {
    return data;
}

inline NullLogData& operator<<(NullLogData& data, [[maybe_unused]] int8_t v) {
    return data;
}

}

