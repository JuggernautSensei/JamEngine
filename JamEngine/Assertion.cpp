#include "pch.h"
#include "Assertion.h"

#include <comdef.h>

#include "Log.h"
#include "Timer.h"
#include "Debug.h"
#include "File.h"
#include "WindowsUtilities.h"
#include "String.h"

namespace Jam::Details
{

namespace
{
    String CreateErrorMessage_(
        const std::source_location& _loc,
        const std::string_view      _msg)
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

void ReportCrash(
    const std::source_location& _loc,
    const std::string_view      _msg)
{
    const String msg = CreateErrorMessage_(_loc, _msg);
    JAM_LOG_CRITICAL(msg);
    std::this_thread::sleep_for(50ms);
    PrintToDebugConsole(msg);   // print to debug console

    // crash dump
    const TimeStamp ts   = TimeStamp::Current();
    const String    path = String::Formatted("CrashDump/{:04}-{:02}-{:02} {:02}-{:02}-{:02}.{:03}.txt", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);
    if (Expect<FileWriter> writer = FileWriter::Open(path))
    {
        writer->WriteString(msg);
        writer->Flush();
    }

    // show msg box
    PrintToMessageBox("Jam engine crash", msg);
}

void ReportAssertion(
    const std::source_location& _loc,
    const std::string_view      _msg)
{
    const String msg = CreateErrorMessage_(_loc, _msg);
    JAM_LOG_ERROR(msg);
    std::this_thread::sleep_for(50ms);
    PrintToDebugConsole(msg);   // print to debug console
}

void ReportHRESULTFailure(
    const std::source_location& _loc,
    const std::string_view      _msg,
    const HRESULT               _hr)
{
    const String hrMsg   = CreateWindowsErrorMessage(_hr, eWindowsErrorCodeType::HRESULT);
    const String fullMsg = String::Formatted("{}\nHRESULT: 0x{:08X} ({})", _msg, _hr, hrMsg);
    ReportCrash(_loc, fullMsg.ToStdStringView());
}

}   // namespace Jam::Details