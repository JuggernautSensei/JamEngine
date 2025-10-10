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

enum class eLogCategory
{
    Core,
    App,
    Gfx,
    Asset,
    Scene,
    Script,
    Audio,
    Physics,
    Input,
    UI,
    Network,
    FileSystem,
};

}   // namespace jam