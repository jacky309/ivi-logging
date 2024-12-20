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

template <typename MapType, class LogType>
LogType& streamMapType(LogType& log, MapType const& v) {
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

template <typename ArrayType, class LogType>
LogType& streamArrayType(LogType& log, ArrayType const& v) {
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

template <typename ElementType, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::vector<ElementType> const& v) {
    return streamArrayType(log, v);
}

template <typename KeyType, typename ValueType, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::map<KeyType, ValueType> const& v) {
    return streamMapType(log, v);
}

template <typename KeyType, typename ValueType, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::unordered_map<KeyType, ValueType> const& v) {
    return streamArrayType(log, v);
}

template <typename ElementType, std::size_t Extent, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::array<ElementType, Extent> const& v) {
    return streamArrayType(log, v);
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::exception const& ex) {
    log << ex.what();
    return log;
}

template <typename ElementType, size_t N, typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, ElementType const (&v)[N]) {
    streamArrayType(log, v);
    return log;
}

template <size_t N, typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, char const (&v)[N]) {
    log << std::string_view{v};
    return log;
}

} // namespace logging

namespace std {

template <typename LogType>
logging::enable_if_logging_type<LogType> endl(LogType&& log) {
    log << "\n";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> hex(LogType&& log) {
    log.setHexEnabled(true);
    return log;
}
template <typename LogType>
logging::enable_if_logging_type<LogType> dec(LogType&& log) {
    log.setHexEnabled(false);
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> ends(LogType&& log) {
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> flush(LogType&& log) {
    return log;
}

} // namespace std

#if __cplusplus >= 202002L
#include <span>
namespace logging {

template <typename ElementType, std::size_t Extent, typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::span<ElementType, Extent> const& v) {
    return streamArrayType(log, v);
}

} // namespace logging
#endif
