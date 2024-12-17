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
std::enable_if_t<std::is_base_of_v<logging::LogData, LogDataType>, LogDataType&> operator<<(LogDataType& log, StringBuilder const& b) {
    log << b.str();
    return log;
}

template <typename EnumType, typename LogDataType>
std::enable_if_t<std::is_base_of_v<logging::LogData, LogDataType> and std::is_enum_v<EnumType>, LogDataType&> operator<<(LogDataType& log, EnumType const& b) {
    log << static_cast<int>(b);
    return log;
}

} // namespace logging
