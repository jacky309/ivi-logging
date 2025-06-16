/**
 * This file define how to output standard types to the logging system
 */
#pragma once

#include "ivi-logging-common.h"
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
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
    log << "[ ";
    for (auto& element : v) {
        if (not isFirst) {
            log << ", ";
        }
        isFirst = false;
        log << element;
    }
    log << " ]";
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

template <typename ElementType, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::set<ElementType> const& v) {
    return streamArrayType(log, v);
}

template <typename ElementType, std::size_t Extent, class LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::array<ElementType, Extent> const& v) {
    return streamArrayType(log, v);
}

template <typename... VariantTypes, typename LogType, typename DisableInvalidVariants = std::enable_if_t<sizeof...(VariantTypes) != 0, void>>
::logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::variant<VariantTypes...> const& value) {
    std::visit(
        [&](auto const& v) {
            log << v;
        },
        value);

    return log;
}

template <typename Type, typename LogType>
::logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::optional<Type> const& value) {
    if (value.has_value()) {
        log << value.value();
    } else {
        log << "{}";
    }

    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::system_clock::time_point const& timePoint) {
    std::time_t tm = std::chrono::system_clock::to_time_t(timePoint);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&tm), "%Ec");
    log << oss.str();
    return log;
}

#if __cplusplus >= 202002L
template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::years const& value) {
    log << value.count() << "yr";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::months const& value) {
    log << value.count() << "mo";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::weeks const& value) {
    log << value.count() << "wk";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::days const& value) {
    log << value.count() << "d";
    return log;
}
#endif

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::hours const& value) {
    log << value.count() << "h";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::minutes const& value) {
    log << value.count() << "min";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::seconds const& value) {
    log << value.count() << "s";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::milliseconds const& value) {
    log << value.count() << "ms";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::microseconds const& value) {
    log << value.count() << "us";
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::chrono::nanoseconds const& value) {
    log << value.count() << "ns";
    return log;
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
