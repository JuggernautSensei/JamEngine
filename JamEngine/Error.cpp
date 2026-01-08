#include "pch.h"

#include "Error.h"

#include "Log.h"
#include "Timer.h"
#include <comdef.h>

#include "Debug.h"
#include "FileIOUtilities.h"
#include "Magics.h"
#include "WindowsUtilities.h"

namespace jam::details
{
[[nodiscard]] static std::string FormatErrorString(
    const std::source_location& _loc,
    const StringView            _userReport)
{
    const TimeStamp ts = TimeStamp::Current();
    return std::format(
        "\n"
        "====================================\n"
        "error occurred !\n"
        "------------------------------------\n"
        "architecture: {}\n"
        "cpu: {}\n"
        "platform: {}\n"
        "compiler: {}\n"
        "language: {}\n"
        "text encoding: {}\n"
        "graphics api: {}\n"
        "engine version: {} {}\n"
        "time: {:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}\n"
#ifdef JAM_DEBUG
        "debug build enabled\n"
#endif
        "------------------------------------\n"
        "file name: {}\n"
        "function name: {}\n"
        "line: {}\n"
        "column: {}\n"
        "------------------------------------\n"
        "user report: \n"
        "{}\n"
        "====================================\n",
        JAM_ARCH_NAME,
        JAM_CPU_NAME,
        JAM_PLATFORM_NAME,
        JAM_COMPILER_NAME,
        JAM_CPP_NAME,
        JAM_TEXT_ENCODING_NAME,
        JAM_GFX_API_NAME,
        JAM_ENGINE_NAME,
        JAM_VERSION_NAME,
        ts.year,
        ts.month,
        ts.day,
        ts.hour,
        ts.minute,
        ts.second,
        ts.millisecond,
        _loc.file_name(),
        _loc.function_name(),
        _loc.line(),
        _loc.column(),
        _userReport);
}

void AssertFail(
    const std::source_location& _loc,
    const StringView            _str)
{
    const std::string msg = FormatErrorString(_loc, _str);
    Log::Critical(msg);
    std::this_thread::sleep_for(100ms);   // wait for logging
    PrintToDebugConsole(msg);             // print to debug console
}

void Crash(
    const std::source_location& _loc,
    const StringView            _str)
{
    const std::string msg = FormatErrorString(_loc, _str);
    Log::Critical(msg);
    Log::Shutdown();            // shutdown for filesink & flush
    PrintToDebugConsole(msg);   // print to debug console

    // crash dump
    const TimeStamp ts   = TimeStamp::Current();
    const fs::path  path = std::format("{}\\{:04}.{:02}.{:02}\\{:02}h {:02}m {:02}.{:03}s.txt", k_crashDumpsDirName, ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);

    SaveTextFile(path, msg, eSaveFileFlags_CreateDirs);
    MessageBoxA(NULL, msg.c_str(), "jam engine crash", MB_OK | MB_ICONERROR);
    std::abort();
}

void CrashWithHRESULT(
    const std::source_location& _loc,
    StringView                  _func,
    HRESULT                     _hr)
{
    Crash(_loc, "\"{}\" failed!\nerror code: {:#X}\n{}", _func, _hr, CreateErrorMessageFromHRESULT(_hr));
}

}   // namespace jam::details
