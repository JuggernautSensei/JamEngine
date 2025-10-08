#include "pch.h"

#include "Error.h"
#include "Log.h"
#include "Timer.h"
#include <comdef.h>
using namespace jam;

namespace
{

JAM_NODISCARD std::string CreateErrorMessage(std::string_view _msg, const std::source_location& _loc)
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
        "{}\n"
        "====================================\n",
        JAM_ARCH_NAME,
        JAM_CPU_NAME,
        JAM_PLATFORM_NAME,
        JAM_COMPILER_NAME,
        JAM_CPP_NAME,
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

thread_local std::string g_lastErrorMessage;

}   // namespace

namespace jam
{

namespace details
{

    void Assert(const std::string_view _msg, const std::source_location& _loc)
    {
        std::string msg = CreateErrorMessage(_msg, _loc);
        log::Critical(msg);
        log::Flush();
        OutputDebugStringA(msg.c_str());

        JAM_DEBUG_BREAK;
    }

    void Crash(const std::string_view _msg, const std::source_location& _loc)
    {
        // 로깅 + 메시지 박스 + crash dump 저장 + 강제 종료
        std::string msg = CreateErrorMessage(_msg, _loc);
        log::Critical(msg);
        log::Flush();
        OutputDebugStringA(msg.c_str());

        // crash dump 저장
        TimeStamp stamp    = TimeStamp::Current();
        fs::path  dir      = std::format("crash dump\\{:04}.{:02}.{:02}", stamp.year, stamp.month, stamp.day);
        fs::path  filename = std::format("{:02}h {:02}m {:02}.{:03}s.txt", stamp.hour, stamp.minute, stamp.second, stamp.millisecond);
        fs::create_directories(dir);
        std::fstream fs { dir / filename, std::ios::out };
        if (fs.is_open())
        {
            fs << msg << "\n";
        }
        MessageBox(NULL, msg.c_str(), "jam engine crash", MB_OK | MB_ICONERROR);
        std::abort();
    }

    void CrashWithCom(std::string_view _msg, HRESULT _hr, const std::source_location& _loc)
    {
        _com_error  err { _hr };
        LPCTSTR     errMsg  = err.ErrorMessage();
        std::string fullMsg = std::format("{}\nHRESULT: {:#X}\nError message: {}", _msg, _hr, errMsg ? errMsg : "Unknown error");
        Crash(fullMsg, _loc);
    }
}   // namespace details

void SetErrorString(const std::string_view _msg)
{
    g_lastErrorMessage = _msg;
}

std::string_view GetErrorString()
{
    return g_lastErrorMessage;
}

}   // namespace jam