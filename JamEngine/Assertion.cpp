#include "pch.h"
#include "Assertion.h"

#include <comdef.h>

#include "LogSystem.h"
#include "Timer.h"
#include "File.h"
#include "WindowsUtilities.h"
#include "String.h"

namespace Jam::Details
{

namespace
{
    [[nodiscard]] String MakeErrorMessage_(
        const std::source_location& _loc,
        const char*                 _msg)
    {
        const TimeStamp ts = TimeStamp::Current();
        return String::Formatted(
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
            _msg);
    }

}   // namespace

void ReportCrash__(
    const std::source_location& _loc,
    const char*                 _msg)
{
    const String msg = MakeErrorMessage_(_loc, _msg);
    JAM_LOG_CRITICAL(msg);
    spdlog::apply_all([](auto _logger) { _logger->flush(); });
    spdlog::shutdown();   // immediately flush and close all loggers to release file handles, etc.

    // crash dump
    const TimeStamp ts   = TimeStamp::Current();
    const String    path = String::Formatted("CrashDump/{:04}-{:02}-{:02} {:02}-{:02}-{:02}.{:03}.txt", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);
    if (Expect<FileWriter> writer = FileWriter::Open(path))
    {
        writer->Write(msg);
        writer->Flush();
    }

    // alert
    const WString mbMsg = WString::Encoded(msg.ToStringView());
    ::MessageBoxW(nullptr, mbMsg.GetPtr(), L"Fatal Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

void ReportHRESULTFailure_(
    const std::source_location& _loc,
    const std::string_view      _func,
    const HRESULT               _hr)
{
    const String hrMsg   = MakeWindowsErrorMessage(_hr, eWindowsErrorCode::HRESULT);
    const String fullMsg = String::Formatted("HRESULT Failed! {}\nHRESULT: 0x{:08X} ({})", _func, _hr, hrMsg);
    ReportCrash__(_loc, fullMsg.GetPtr());
}

}   // namespace Jam::Details