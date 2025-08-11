#include "pch.h"

#include "Log.h"

#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace jam
{

void Log::Initialize()
{
    // 로그 스레드 풀
    spdlog::init_thread_pool(8192, 1);

    // 로그 패턴
    std::string logPattern { k_defaultLogPattern };

    // 콘솔 싱크 생성
    const spdlog::sink_ptr consoleSink = MakeRef<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(std::string(k_defaultLogPattern));

    // 파일 싱크 생성
    TimeStamp              timeStamp = TimeStamp::Create();
    std::string            filepath  = std::format(R"({0}\{1}.{2}.{3}\log_{1}.{2}.{3}_{4}h{5}m{6}s.txt)", k_logDirectory, timeStamp.year, timeStamp.month, timeStamp.day, timeStamp.hour, timeStamp.minute, timeStamp.second);
    const spdlog::sink_ptr fileSink  = MakeRef<spdlog::sinks::basic_file_sink_mt>(filepath);
    fileSink->set_pattern(std::string(k_defaultLogPattern));

    // 메모리 싱크 생성 (캐싱)
    s_memorySink = MakeRef<MemorySinkMT>();
    s_memorySink->set_pattern(std::string(k_memoryLogPattern));

    // 싱크를 결합하여 로거 생성
    spdlog::sinks_init_list   sinks      = { consoleSink, fileSink, s_memorySink };
    Ref<spdlog::async_logger> coreLogger = MakeRef<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    coreLogger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(coreLogger);   // 로거 등록

    // 초기화 종료
    Trace("log system initialized");
}

void Log::Shutdown()
{
    Trace("log system shutdown");
    spdlog::shutdown();
}

}   // namespace jam