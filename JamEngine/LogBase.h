#pragma once
#include <spdlog/spdlog.h>

namespace jam
{

enum class eLogLevel
{
    Trace,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

}