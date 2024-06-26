#pragma once

#include "dlt_user.h"

#include "ivi-logging-common.h"

namespace logging {

class DltLogData;

class DltContextClass : public LogContextBase, private DltContext {

public:
	typedef DltLogData LogDataType;

	DltContextClass() {
	}

	~DltContextClass() {
		dlt_unregister_context(this);
	}

	void setParentContext(LogContextCommon& context) {
		m_context = &context;
	}

	bool isEnabled(LogLevel logLevel) const;

	static DltLogLevelType getDLTLogLevel(LogLevel level) {
		DltLogLevelType v = DLT_LOG_DEFAULT;
		switch (level) {
		case LogLevel::Debug : v = DLT_LOG_DEBUG; break;
		case LogLevel::Info : v = DLT_LOG_INFO; break;
		case LogLevel::Warning : v = DLT_LOG_WARN; break;
		case LogLevel::Fatal : v = DLT_LOG_FATAL; break;
		case LogLevel::Error : v = DLT_LOG_ERROR; break;
		case LogLevel::Verbose : v = DLT_LOG_VERBOSE; break;
		case LogLevel::None : v = DLT_LOG_OFF; break;
		default : v = DLT_LOG_DEFAULT; break;
		}
		return v;
	}

	void registerContext() {
		LogContextBase::registerContext();

		if ( !isDLTAppRegistered() ) {
			registerDLTApp( s_pAppLogContext->m_id.c_str(), s_pAppLogContext->m_description.c_str() );
			isDLTAppRegistered() = true;
		}

		dlt_register_context( this, m_context->getID(), m_context->getDescription() );
	}

	static bool& isDLTAppRegistered() {
		static bool m_appRegistered = false;
		return m_appRegistered;
	}

	/**
	 * Register the application.
	 */
	static void registerDLTApp(const char* id, const char* description) {

		pid_t pid = getpid();
		char descriptionWithPID[1024];
		snprintf(descriptionWithPID, sizeof(descriptionWithPID), "PID:%i / %s", pid, description);

		auto dltCode = dlt_register_app(id, descriptionWithPID);

		// TODO : The piece of code below would be useful if the DLT library didn't always return 0
		if (dltCode != 0 ) {
			char pidAsHexString[5];
			snprintf(pidAsHexString, sizeof(pidAsHexString), "%X", pid);
			dltCode = dlt_register_app(pidAsHexString, descriptionWithPID);
		}
		isDLTAppRegistered() = true;

		// TODO : caution is needed when forking a process if the connection to the DLT daemon is active.
		// We can probably use pthread_atfork() to disconnect before the fork, and reconnect after
//		int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

	}

private:
	LogContextCommon* m_context = nullptr;

	friend class DltLogData;
};


class DltLogData : public LogData, public DltContextData {

public:
	typedef DltContextClass ContextType;

	DltLogData() {
	}

	void init(DltContextClass& context, const LogInfo& data) {
		m_data = data;
		m_context = &context;
		auto dltLogLevel = m_context->getDLTLogLevel( getData().getLogLevel() );
		m_enabled = (dlt_user_log_write_start(m_context, this, dltLogLevel) > 0);
	}

	virtual ~DltLogData() {
		if (isEnabled()) {
			if (m_context->isSourceCodeLocationInfoEnabled()) {
				dlt_user_log_write_utf8_string(this, "                                                    | ");
				if (getData().getFileName() != nullptr) dlt_user_log_write_utf8_string( this, getData().getFileName() );
				if (getData().getLineNumber() != -1) dlt_user_log_write_uint32( this, getData().getLineNumber() );
				if (getData().getPrettyFunction() != nullptr) dlt_user_log_write_utf8_string(
						this, getData().getPrettyFunction() );
			}

			if (m_context->isThreadInfoEnabled()) {
				dlt_user_log_write_string(this, "ThreadID");
				dlt_user_log_write_uint8( this, getThreadInformation().getID() );
				dlt_user_log_write_string( this, getThreadInformation().getName() );
			}

			dlt_user_log_write_finish(this);
		}
	}

	const LogInfo& getData() const {
		return m_data;
	}

	bool isEnabled() const {
		return m_enabled;
	}

	template<typename ... Args>
	void writeFormatted(const char* format, Args ... args) {
		if (m_enabled) {
			std::array<char, 65536> buffer;

#pragma GCC diagnostic push
			// Make sure GCC does not complain about not being able to check the format string since it is no literal string
#pragma GCC diagnostic ignored "-Wformat-security"
			snprintf(buffer.data(), std::tuple_size_v<decltype(buffer)>, format, args ...);
#pragma GCC diagnostic pop

			dlt_user_log_write_utf8_string(this, buffer.data());
		}
	}

	void setHexEnabled(bool enabled) {
		m_hexEnabled = enabled;
	}

	bool isHexEnabled() const {
		return m_hexEnabled;
	}

private:
	DltContextClass* m_context = nullptr;
	LogInfo m_data;
	bool m_enabled = false;
	bool m_hexEnabled{false};
};

inline bool DltContextClass::isEnabled(LogLevel logLevel) const {
#ifdef DLT_2_9
	DltContextData d;
	return dlt_user_log_write_start( this, &d, getDLTLogLevel(logLevel) );
#else
	auto dltLogLevel = getDLTLogLevel(logLevel);
	return ( (this)->log_level_ptr && ( (dltLogLevel) <= (int)*( (this)->log_level_ptr ) ) &&
		 ( (dltLogLevel) != 0 ) );    // TODO: get that expression from the DLT itself
#endif
}

inline DltLogData& operator<<(DltLogData& data, bool v) {
	dlt_user_log_write_bool(&data, v);
	return data;
}

inline DltLogData& operator<<(DltLogData& data, const char* v) {
	dlt_user_log_write_utf8_string(&data, v);
	return data;
}

template<size_t N>
inline DltLogData& operator<<(DltLogData& data, const char (&v)[N]) {
   data << (const char*) v;
   return data;
}

inline DltLogData& operator<<(DltLogData& data, std::string_view v) {
	if ( data.isEnabled() )
		data.writeFormatted("%.*s", static_cast<int>(v.length()), v.data());
	return data;
}

inline DltLogData& operator<<(DltLogData& data, const std::string& v) {
	data << v.c_str();
	return data;
}

inline DltLogData& operator<<(DltLogData& data, float f) {
	// we assume a float is 32 bits
	dlt_user_log_write_float32(&data, f);
	return data;
}


// TODO : strangely, it seems like none of the types defined in "stdint.h" is equivalent to "long int" on a 32 bits platform
#if __WORDSIZE == 32
inline DltLogData& operator<<(DltLogData& data, long int v) {
	dlt_user_log_write_int32(&data, v);
	return data;
}
inline DltLogData& operator<<(DltLogData& data, unsigned long int v) {
	dlt_user_log_write_uint32(&data, v);
	return data;
}
#endif

inline DltLogData& operator<<(DltLogData& data, double f) {
	// we assume a double is 64 bits
	dlt_user_log_write_float64(&data, f);
	return data;
}

inline DltLogData& operator<<(DltLogData& data, uint64_t v) {
	if (data.isHexEnabled()) {
    	dlt_user_log_write_uint64_formatted(&data, v, DLT_FORMAT_HEX64);
	} else {
		dlt_user_log_write_uint64(&data, v);
	}
	return data;
}

inline DltLogData& operator<<(DltLogData& data, int64_t v) {
	dlt_user_log_write_int64(&data, v);
	return data;
}

inline DltLogData& operator<<(DltLogData& data, uint32_t v) {
	if (data.isHexEnabled()) {
    	dlt_user_log_write_uint32_formatted(&data, v, DLT_FORMAT_HEX32);
	} else {
		dlt_user_log_write_uint32(&data, v);
	}
	return data;
}

inline DltLogData& operator<<(DltLogData& data, int32_t v) {
	dlt_user_log_write_int32(&data, v);
	return data;
}

inline DltLogData& operator<<(DltLogData& data, uint16_t v) {
	if (data.isHexEnabled()) {
    	dlt_user_log_write_uint16_formatted(&data, v, DLT_FORMAT_HEX16);
	} else {
		dlt_user_log_write_uint16(&data, v);
	}
	return data;
}

inline DltLogData& operator<<(DltLogData& data, int16_t v) {
	dlt_user_log_write_int16(&data, v);
	return data;
}

inline DltLogData& operator<<(DltLogData& data, uint8_t v) {
	if (data.isHexEnabled()) {
    	dlt_user_log_write_uint8_formatted(&data, v, DLT_FORMAT_HEX8);
	} else {
		dlt_user_log_write_uint8(&data, v);
	}
	return data;
}

inline DltLogData& operator<<(DltLogData& data, int8_t v) {
	dlt_user_log_write_int8(&data, v);
	return data;
}

}
