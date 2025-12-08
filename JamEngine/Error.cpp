#include "pch.h"

#include "Error.h"

#include "FString.h"
#include "Log.h"
#include "StringUtilities.h"
#include "Timer.h"
#include <comdef.h>

namespace jam
{

[[nodiscard]] static std::string CreateErrorMessage(std::string_view _msg, const std::source_location& _loc)
{
    TimeStamp   stamp   = TimeStamp::Current();
    std::string timeStr = std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second, stamp.millisecond);

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
        "is debug: {}\n"
        "time: {}\n"
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
#ifdef JAM_DEBUG
        "true",
#else
        "false",
#endif
        timeStr,
        _loc.file_name(),
        _loc.function_name(),
        _loc.line(),
        _loc.column(),
        _msg);
}

namespace details
{

    void AssertFail(const std::source_location& _loc, const std::string_view _msg)
    {
        const std::string msg = CreateErrorMessage(_msg, _loc);
        Log::Critical(msg);
        std::this_thread::sleep_for(100ms);   // wait for logging
        OutputDebugStringA(msg.c_str());
    }

    void Crash(const std::source_location& _loc, const std::string_view _msg)
    {
        std::string msg = CreateErrorMessage(_msg, _loc);
        Log::Critical(msg);
        std::this_thread::sleep_for(100ms);   // wait for logging
        OutputDebugStringA(msg.c_str());

        // crash dump
        const TimeStamp timeStamp = TimeStamp::Current();
        const fs::path  dir       = std::format("CrashDump\\{:04}.{:02}.{:02}", timeStamp.year, timeStamp.month, timeStamp.day);
        const fs::path  filename  = std::format("{:02}h {:02}m {:02}.{:03}s.txt", timeStamp.hour, timeStamp.minute, timeStamp.second, timeStamp.millisecond);

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

    void CrashCom(const std::source_location& _loc, const char* _func, HRESULT _hr)
    {
        const _com_error err { _hr };
#ifdef JAM_TEXT_ENCODING_UNICODE
        const std::string msg = ToUTF8(err.ErrorMessage());
#else
        const std::string msg = err.ErrorMessage();
#endif
        Crash(_loc, "\"{}\" failed!\nerror code: {:#X}\n{}", _func, _hr, msg);
    }
}   // namespace details

static FString<1024> g_errorString;

void SetErrorString(const std::string_view _msg)
{
    g_errorString = _msg;
}

const char* GetErrorString()
{
    return g_errorString.c_str();
}

std::string CreateComErrorString(const HRESULT _hr)
{
    _com_error err(_hr);
#ifdef JAM_TEXT_ENCODING_UNICODE
    std::wstring_view wmsg = err.ErrorMessage();
    return ToUTF8(wmsg);
#else
    return err.ErrorMessage();
#endif
}

}   // namespace jam