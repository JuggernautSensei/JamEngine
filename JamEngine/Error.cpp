#include "pch.h"

#include "Error.h"
#include "Logger.h"
#include "StringUtilities.h"
#include "Timer.h"
#include <comdef.h>
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

JAM_NODISCARD static std::string CreateErrorMessage(std::string_view _msg, const std::source_location& _loc)
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

thread_local static std::string g_lastErrorMessage;

////////////////////////////////////////////////////////////

namespace jam
{

namespace details
{

    void ReportAssertFailed(const std::source_location& _loc, const std::string_view _msg)
    {
        std::string msg = CreateErrorMessage(_msg, _loc);
        logger::Critical(msg);
        logger::Flush();
        OutputDebugStringA(msg.c_str());
    }

    void ReportCrash(const std::source_location& _loc, const std::string_view _msg)
    {
        // 로깅 + 메시지 박스 + crash dump 저장 + 강제 종료
        std::string msg = CreateErrorMessage(_msg, _loc);
        logger::Critical(msg);
        logger::Flush();
        OutputDebugStringA(msg.c_str());

        // crash dump 저장
        TimeStamp stamp    = TimeStamp::Current();
        fs::path  dir      = std::format("crash dump\\{:04}.{:02}.{:02}", stamp.year, stamp.month, stamp.day);
        fs::path  filename = std::format("{:02}h {:02}m {:02}.{:03}s.txt", stamp.hour, stamp.minute, stamp.second, stamp.millisecond);

        // 디렉토리 생성 및 파일 쓰기
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (!ec)
        {
            std::ofstream fs { dir / filename, std::ios::out };
            if (fs.is_open())
            {
                fs << msg << "\n";
            }
        }

        // 메시지 박스 출력
        MessageBoxA(NULL, msg.c_str(), "jam engine crash", MB_OK | MB_ICONERROR);
    }

}   // namespace details

void SetErrorString(const std::string_view _msg)
{
    g_lastErrorMessage = _msg;
}

std::string details::GetComErrorMessage(const HRESULT _hr)
{
    _com_error   err { _hr };
    const TCHAR* msg = err.ErrorMessage();
    if (msg)
    {
        return "Unknown error";
    }

#ifdef UNICODE
    return ToUTF8(msg);
#else
    return msg;
#endif
}

std::string_view GetErrorString()
{
    return g_lastErrorMessage;
}

}   // namespace jam