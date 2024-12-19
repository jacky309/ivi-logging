/**
 * This file define how to output standard types to the logging system
 */
#pragma once

#include "ivi-logging-common.h"
#include <exception>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace logging {

template <typename MapType, class LogDataType>
LogDataType& streamMapType(LogDataType& log, MapType const& v) {
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

template <typename ArrayType, class LogDataType>
LogDataType& streamArrayType(LogDataType& log, ArrayType const& v) {
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

template <typename ElementType, class LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::vector<ElementType> const& v) {
    return streamArrayType(log, v);
}

template <typename KeyType, typename ValueType, class LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::map<KeyType, ValueType> const& v) {
    return streamMapType(log, v);
}

template <typename KeyType, typename ValueType, class LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType& log, std::unordered_map<KeyType, ValueType> const& v) {
    return streamArrayType(log, v);
}

template <typename ElementType, std::size_t Extent, class LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::array<ElementType, Extent> const& v) {
    return streamArrayType(log, v);
}

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::exception const& ex) {
    log << ex.what();
    return log;
}

} // namespace logging

namespace std {

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> endl(LogDataType&& log) {
    log << "\n";
    return log;
}

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> hex(LogDataType&& log) {
    log.setHexEnabled(true);
    return log;
}
template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> dec(LogDataType&& log) {
    log.setHexEnabled(false);
    return log;
}

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> ends(LogDataType&& log) {
    // TODO : implement
    return log;
}

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> flush(LogDataType&& log) {
    // TODO : implement
    return log;
}

} // namespace std

#if __cplusplus >= 202002L
#include <span>
namespace logging {

template <typename ElementType, std::size_t Extent, typename LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType& log, std::span<ElementType, Extent> const& v) {
    return streamArrayType(log, v);
}

} // namespace logging
#endif
