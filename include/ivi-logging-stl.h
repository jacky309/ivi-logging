/**
 * This file define how to output standard types to the logging system
 */
#pragma once

#include "ivi-logging-common.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <exception>
#include <type_traits>

namespace logging {

template<typename MapType, class LogDataType = logging::LogData>
LogDataType& streamMapType(LogDataType& log, const MapType& v) {
	bool isFirst = true;
	log << " [ ";
	for (auto& element : v) {
		if (not isFirst) {
			log << ", ";
		}
		isFirst = false;
		log << "{ ";
		log << element.first;
		log << "=";
		log << element.second;
		log << " }";
	}
	log << " ] ";
	return log;
}

template<typename ArrayType, class LogDataType = logging::LogData>
LogDataType& streamArrayType(LogDataType& log, const ArrayType& v) {
	bool isFirst = true;
	log << " [ ";
	for (auto& element : v) {
		if (not isFirst) {
			log << ", ";
		}
		isFirst = false;
		log << element;
	}
	log << " ] ";
	return log;
}

template<typename ElementType, class LogDataType = logging::LogData, typename =
		 typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::vector<ElementType>& v) {
	return streamArrayType(log, v);
}

template<typename KeyType, typename ValueType, class LogDataType = logging::LogData, typename =
		 typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::map<KeyType, ValueType>& v) {
	return streamMapType(log, v);
}

template<typename KeyType, typename ValueType, class LogDataType = logging::LogData, typename =
		 typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::unordered_map<KeyType, ValueType>& v) {
	return streamArrayType(log, v);
}

template<typename ElementType, std::size_t Extent, class LogDataType = logging::LogData, typename =
		 typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::array<ElementType, Extent>& v) {
	return streamArrayType(log, v);
}

template<typename LogDataType, typename =
         typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::exception& ex) {
    log << ex.what();
    return log;
}

}

namespace std {

template<typename LogDataType, typename =
		 typename std::enable_if<std::is_base_of<logging::LogInfo, LogDataType>::value>::type>
LogDataType&
  endl(LogDataType& log) {
	log << "\n";
	return log;
  }

template<typename LogDataType, typename =
		 typename std::enable_if<std::is_base_of<logging::LogInfo, LogDataType>::value>::type>
LogDataType&
  hex(LogDataType& log) {
	log.setHexEnabled(true);
	return log;
}

template<typename LogDataType, typename =
		 typename std::enable_if<std::is_base_of<logging::LogInfo, LogDataType>::value>::type>
LogDataType&
  dec(LogDataType& log) {
	log.setHexEnabled(false);
	return log;
  }

template<typename LogDataType, typename =
		 typename std::enable_if<std::is_base_of<logging::LogInfo, LogDataType>::value>::type>
LogDataType&
  ends(LogDataType& log) {
	// TODO : implement
	return log;
  }

template<typename LogDataType, typename =
		 typename std::enable_if<std::is_base_of<logging::LogInfo, LogDataType>::value>::type>
LogDataType&
  flush(LogDataType& log) {
	// TODO : implement
	return log;
  }

}

#if __cplusplus >= 202002L
#include <span>
namespace logging {

template<typename ElementType, std::size_t Extent, class LogDataType = logging::LogData, typename =
		 typename std::enable_if<std::is_base_of<logging::LogData, LogDataType>::value>::type>
LogDataType& operator<<(LogDataType& log, const std::span<ElementType, Extent>& v) {
	return streamArrayType(log, v);
}

}
#endif
