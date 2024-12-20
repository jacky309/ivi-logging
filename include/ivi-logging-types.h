/**
 * This file define how to output standard types to the logging system
 */
#pragma once

#include "ivi-logging-common.h"
#include "ivi-logging-stl.h"
#include <type_traits>

namespace logging {

static constexpr char const* NULL_POINTER_STRING = "nullptr";

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, StringBuilder const& b) {
    log << b.str();
    return log;
}

template <typename LogDataType>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::string const& v) {
    log.write(v);
    return log;
}

template <typename TupType, class LogDataType, size_t... I>
void printTuple(TupType const& _tup, LogDataType& log, std::index_sequence<I...>) {
    log << "{";
    (..., (log << (I == 0 ? "" : ", ") << std::get<I>(_tup)));
    log << "}";
}

template <typename LogDataType, typename... TupleTypes>
logging::enable_if_logging_type<LogDataType> operator<<(LogDataType&& log, std::tuple<TupleTypes...> const& value) {
    printTuple(value, log, std::make_index_sequence<sizeof...(TupleTypes)>());
    return log;
}

template <typename EnumType, typename LogDataType>
std::enable_if_t<std::is_enum_v<EnumType>, logging::enable_if_logging_type<LogDataType>> operator<<(LogDataType&& log, EnumType const& b) {
    log << static_cast<int>(b);
    return log;
}

} // namespace logging
