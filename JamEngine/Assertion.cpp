#include "pch.h"
#include "Assertion.h"

#include <comdef.h>

#include "ScratchBuffer.h"
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

    using AssertScratchBuffer = BasicScratchBuffer<8192>;

    void FormatErrorMessage(
        AssertScratchBuffer&        _buf,
        const std::source_location& _loc,
        const std::string_view      _msg)
    {
        const TimeStamp ts = TimeStamp::Current();
        _buf.Format(
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
    AssertScratchBuffer buf;
    FormatErrorMessage(buf, _loc, _msg);

    const StringView msg = buf.ToStringView();
    JAM_LOG.Critical(msg);
    JAM_LOG.Flush();
    PrintToDebugConsole(msg);             // print to debug console

    // crash dump
    const TimeStamp ts   = TimeStamp::Current();
    const Path      path = fmt::format("CrashDump\\{:04}.{:02}.{:02}\\{:02}h {:02}m {:02}.{:03}s.txt", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);
    if (Expect<FileWriter> writer = FileWriter::Open(path, eSaveFileFlags::CreateDirs))
    {
        writer->Write(msg);
        writer->Flush();
    }

    // show message box
    MessageBoxA(NULL, msg.GetPtr(), "Jam engine crash", MB_OK | MB_ICONERROR);
}

void ReportAssertion(
    const std::source_location& _loc,
    const std::string_view      _msg)
{
    AssertScratchBuffer buf;
    FormatErrorMessage(buf, _loc, _msg);
    PrintToDebugConsole(buf.GetPtr());   // print to debug console
}

void ReportHRESULTFailure(
    const std::source_location& _loc,
    std::string_view            _msg,
    const HRESULT               _hr)
{
    ScratchBuffer hrMsgBuf;
    FormatHRESULTMessage(hrMsgBuf, _hr);

    AssertScratchBuffer buf;
    buf.Format("{} (HRESULT: 0x{:08X}) - {}", _msg, NumericCast<UInt32>(_hr), hrMsgBuf.ToStringView());

    ReportCrash(_loc, buf.ToStdStringView());
}

}   // namespace Jam::Details