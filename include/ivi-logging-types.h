/**
 * This file define how to output standard types to the logging system
 */
#pragma once

#include "ivi-logging-common.h"
#include "ivi-logging-stl.h"
#include <type_traits>

namespace logging {

static constexpr char const* NULL_POINTER_STRING = "nullptr";

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, StringBuilder const& b) {
    log << b.str();
    return log;
}

template <typename LogType>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::string const& v) {
    log.write(v);
    return log;
}

template <typename TupType, class LogType, size_t... I>
void printTuple(TupType const& _tup, LogType& log, std::index_sequence<I...>) {
    log << "{";
    (..., (log << (I == 0 ? "" : ", ") << std::get<I>(_tup)));
    log << "}";
}

template <typename LogType, typename... TupleTypes>
logging::enable_if_logging_type<LogType> operator<<(LogType&& log, std::tuple<TupleTypes...> const& value) {
    printTuple(value, log, std::make_index_sequence<sizeof...(TupleTypes)>());
    return log;
}

} // namespace logging
