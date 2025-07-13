#include "pch.h"

#include "Error.h"

#include "Timer.h"

#include <fstream>

namespace jam::detail
{

std::string CreateErrorMessage(const std::string_view _msg, const std::source_location& _loc)
{
    return std::format(
        "\n"
        "====================================\n"
        "error occured !\n"
        "------------------------------------\n"
        "fine name: {}\n"
        "function name: {}\n"
        "line: {}\n"
        "columns: {}\n"
        "------------------------------------\n"
        "{}\n"
        "====================================\n"
        "",
        _loc.file_name(), _loc.function_name(), _loc.line(), _loc.column(), _msg);
}

void ReportCrash(const std::string_view _msg, const std::source_location& _loc)
{
    // 시간 기반 리포트 파일 생성
    constexpr std::string_view k_bufReportDirectory = "bug report";
    TimeStamp                  timeStamp            = TimeStamp::Create();
    const std::string          filepath             = std::format(R"({0}\{1}.{2}.{3}\log_{1}.{2}.{3}_{4}h-{5}m-{6}s.txt)", k_bufReportDirectory, timeStamp.year, timeStamp.month, timeStamp.day, timeStamp.hour, timeStamp.minute, timeStamp.second);

    // 메시지 생성
    const std::string msg = CreateErrorMessage(_msg, _loc);

    // 파일 저장
    std::fstream fs { filepath, std::ios::in };
    if (fs)
    {
        fs << msg << std::endl;
    }

    // 메시지 박스 표시
    MessageBoxA(NULL, msg.c_str(), "jam engine error", MB_OK | MB_ICONERROR);
}

void ReportError(const std::string_view _msg, const std::source_location& _loc)
{
    // 메시지 생성
    const std::string msg = CreateErrorMessage(_msg, _loc);

    // 로깅
    Log::Error(msg);

    // 디버그 모드에서는 로깅을 위해 잠시 슬립
#ifdef _DEBUG
    std::this_thread::sleep_for(50ms);
#endif
}

}   // namespace jam::detail
