#include "pch.h"

#include "Log.h"

#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
using namespace jam;

namespace
{

constexpr const char* kDefaultLogPattern    = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";
constexpr const char* kMemorySinkLogPattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";

std::shared_ptr<MemorySinkMT> s_memorySink = nullptr;

}   // namespace

namespace jam::log
{

void Init()
{
    // 로그 스레드 풀
    spdlog::init_thread_pool(8192, 1);

    // 로그 패턴
    std::string logPattern { kDefaultLogPattern };

    // 콘솔 싱크 생성
    spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(logPattern);

    // 파일 싱크 생성
    TimeStamp timeStamp = TimeStamp::Current();
    fs::path  logDir    = std::format("logs/{:04}-{:02}-{:02}", timeStamp.year, timeStamp.month, timeStamp.day);
    fs::path  filename  = std::format("{:02}h {:02}m {:02}.{:03}s.txt", timeStamp.hour, timeStamp.minute, timeStamp.second, timeStamp.millisecond);
    fs::create_directories(logDir);

    spdlog::sink_ptr fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logDir / filename);
    fileSink->set_pattern(logPattern);

    // 메모리 싱크 생성
    s_memorySink = std::make_shared<MemorySinkMT>();
    s_memorySink->set_pattern(std::string(kMemorySinkLogPattern));

    // 싱크를 결합하여 로거 생성
    spdlog::sinks_init_list               sinks      = { consoleSink, fileSink, s_memorySink };
    std::shared_ptr<spdlog::async_logger> coreLogger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::set_default_logger(coreLogger);   // 로거 등록

#ifdef JAM_DEBUG
    SetLogLevel(eLogLevel::Trace);   // 기본 로그 레벨 설정
#else
    SetLogLevel(eLogLevel::Info);   // 기본 로그 레벨 설정
#endif

    // 초기화 종료
    JAM_LOG_DEBUG("log initialize done.");
}

void Shutdown()
{
    // 자원 정리 전 로그
    JAM_LOG_DEBUG("log system shutdown start.");

    // 클린업
    s_memorySink.reset();
    spdlog::shutdown();
}

void SetLogLevel(const eLogLevel _level)
{
    constexpr spdlog::level::level_enum kLogLevelTable[EnumCount<eLogLevel>()] = {
        spdlog::level::trace,
        spdlog::level::info,
        spdlog::level::warn,
        spdlog::level::err,
        spdlog::level::critical,
        spdlog::level::off
    };

    spdlog::set_level(kLogLevelTable[EnumToInt(_level)]);
}

void Flush()
{
    spdlog::default_logger()->flush();
}

MemorySinkMT::ContainerT::const_iterator log::GetMemoryLogBeginIter()
{
    return s_memorySink->GetBeginIter();
}

MemorySinkMT::ContainerT::const_iterator log::GetMemoryLogEndIter()
{
    return s_memorySink->GetEndIter();
}

size_t log::GetMemoryLogCount()
{
    return s_memorySink->GetCount();
}

void log::ClearMemoryLog()
{
    s_memorySink->Clear();
}
}   // namespace jam::log