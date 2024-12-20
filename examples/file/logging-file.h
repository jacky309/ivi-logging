#pragma once

#include "ivi-logging-file.h"
#include "ivi-logging.h"

// We reuse the default configuration of ivi-logging, but we add our own file logging backend
using LogContext = logging::DefaultLogContext::Extension<logging::FileLogContext, logging::FileLogContext::LogDataType>;
