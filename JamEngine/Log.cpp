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
    JAM_ASSERT(s_coreLogger == nullptr, "Log system already initialized");

    // 로그 스레드 풀
    spdlog::init_thread_pool(8192, 1);

    // 콘솔 싱크 생성
    const spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(std::string(k_logPattern));

    // 파일 싱크 생성
    TimeStamp              timeStamp = TimeStamp::Create();
    std::string            filepath  = std::format(R"({0}\{1}.{2}.{3}\log_{1}.{2}.{3}_{4}h-{5}m-{6}s.txt)", k_logDirectory, timeStamp.year, timeStamp.month, timeStamp.day, timeStamp.hour, timeStamp.minute, timeStamp.second);
    const spdlog::sink_ptr fileSink  = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    // 두 싱크를 결합하여 로거 생성
    std::vector<spdlog::sink_ptr>               sinks { consoleSink, fileSink };
    const std::shared_ptr<spdlog::async_logger> logger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);

    // 로거 등록
    s_coreLogger = logger;
    spdlog::flush_on(spdlog::level::trace);

    // 초기화 종료
    JAM_ASSERT(s_coreLogger, "Log system initialization failed");
    Trace("log system initialized");
}

void Log::Shutdown()
{
    JAM_ASSERT(s_coreLogger, "Log system not initialized");
    Trace("log system shutdown");
    spdlog::shutdown();
}

}   // namespace jam