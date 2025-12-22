#include "pch.h"

#include "Error.h"

#include "Log.h"
#include "StringUtilities.h"
#include "Timer.h"
#include <comdef.h>

namespace jam::details
{

[[nodiscard]] static std::string FormatErrorMessage(
    const std::source_location& _loc,
    const StringView            _str)
{
    const TimeStamp ts      = TimeStamp::Current();
    std::string     strTime = std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);

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
        "time: {}\n"
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
        strTime,
        _loc.file_name(),
        _loc.function_name(),
        _loc.line(),
        _loc.column(),
        _str);
}

void AssertFail(
    const std::source_location& _loc,
    const StringView            _str)
{
    const std::string msg = FormatErrorMessage(_loc, _str);
    Log::Critical(msg);
    std::this_thread::sleep_for(100ms);   // wait for logging
    OutputDebugStringA(msg.c_str());
}

void Crash(
    const std::source_location& _loc,
    const StringView            _str)
{
    const std::string msg = FormatErrorMessage(_loc, _str);
    Log::Critical(msg);
    Log::Shutdown();   // shutdown for filesink & flush
    OutputDebugStringA(msg.c_str());

    // crash dump
    const TimeStamp ts       = TimeStamp::Current();
    const fs::path  dir      = std::format("CrashDump\\{:04}.{:02}.{:02}", ts.year, ts.month, ts.day);
    const fs::path  filename = std::format("{:02}h {:02}m {:02}.{:03}s.txt", ts.hour, ts.minute, ts.second, ts.millisecond);

    try
    {
        fs::create_directories(dir);
        std::ofstream fs { dir / filename, std::ios::out };
        if (fs)
        {
            fs << msg << "\n";
        }
    }
    catch (...)
    {
        JAM_NOOP;   // ignore
    }

    // show message box
    MessageBoxA(NULL, msg.c_str(), "jam engine crash", MB_OK | MB_ICONERROR);
    std::abort();
}

void CrashWithComHint(
    const std::source_location& _loc,
    StringView                  _func,
    HRESULT                     _hr)
{
    Crash(_loc, "\"{}\" failed!\nerror code: {:#X}\n{}", _func, _hr, StringFrom(_hr));
}

}   // namespace jam::details

namespace jam
{

std::string StringFrom(
    const HRESULT _hr)
{
    const _com_error err(_hr);
#ifdef JAM_TEXT_ENCODING_UNICODE
    const WStringView wmsg = err.ErrorMessage();
    return ToUtf8(wmsg);
#else
    return err.ErrorMessage();
#endif
}

}   // namespace jam