#include "pch.h"

#include "Logger.h"

#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

constexpr const char* const k_defaultLogPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";

struct LogContext
{
    void UpateLogger() const
    {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(sinkMap.size());
        for (const std::shared_ptr<spdlog::sinks::sink>& sink: sinkMap | std::views::values)
        {
            sinks.push_back(sink);
        }
        std::shared_ptr<spdlog::async_logger> newLogger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        spdlog::set_default_logger(newLogger);
    }

    HashMap<std::string, spdlog::sink_ptr> sinkMap;
};
static LogContext g_loggerCtx;

//////////////////////////////////////////////////////////////

namespace jam::logger
{

void Init()
{
    // 기본적으로 비동기 로깅 지원
    spdlog::init_thread_pool(8192, 1);

    // 파일 싱크 이름 생성
    TimeStamp   time     = TimeStamp::Current();
    std::string filename = std::format("logs/{:04}-{:02}-{:02}/{:02}h {:02}m {:02}.{:03}s.txt", time.year, time.month, time.day, time.hour, time.minute, time.second, time.millisecond);

    spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::sink_ptr fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);

    consoleSink->set_level(spdlog::level::debug);

    AddSink(k_consoleSinkName, consoleSink);
    AddSink(k_fileSinkName, fileSink);

    SetLogLevel(eLogLevel::Trace);
    SetLogPattern(k_defaultLogPattern);
    JAM_LOGGER_DEBUG("log initialize done.");
}

void Shutdown()
{
    JAM_LOGGER_DEBUG("logger shutdown start.");
    g_loggerCtx.sinkMap.clear();
    spdlog::shutdown();
}

void AddSink(std::string_view _sinkName, const std::shared_ptr<spdlog::sinks::sink>& _sink)
{
    auto it = g_loggerCtx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it == g_loggerCtx.sinkMap.end(), "Sink already exists: {}", _sinkName);
    g_loggerCtx.sinkMap[std::string(_sinkName)] = _sink;
    g_loggerCtx.UpateLogger();
}

void RemoveSink(std::string_view _sinkName)
{
    auto it = g_loggerCtx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_loggerCtx.sinkMap.end(), "Sink not found: {}", _sinkName);
    g_loggerCtx.sinkMap.erase(it);
    g_loggerCtx.UpateLogger();
}

const std::shared_ptr<spdlog::sinks::sink>& GetSink(std::string_view _sinkName)
{
    auto it = g_loggerCtx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_loggerCtx.sinkMap.end(), "Sink not found: {}", _sinkName);
    return it->second;
}

void SetLogLevel(eLogLevel _level)
{
    spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
    spdlog::set_level(level);
}

void SetLogLevel(std::string_view _sinkName, eLogLevel _level)
{
    auto it = g_loggerCtx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_loggerCtx.sinkMap.end(), "Sink not found: {}", _sinkName);
    spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
    it->second->set_level(level);
}

void SetLogPattern(std::string_view _sinkName, const std::string_view _pattern)
{
    auto it = g_loggerCtx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_loggerCtx.sinkMap.end(), "Sink not found: {}", _sinkName);
    it->second->set_pattern(std::string(_pattern));
}

void SetLogPattern(const std::string_view _pattern)
{
    spdlog::set_pattern(std::string(_pattern));
}

void Flush()
{
    spdlog::default_logger()->flush();
}

}   // namespace jam::logger
