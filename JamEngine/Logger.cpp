#include "pch.h"

#include "Logger.h"
#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace jam::logger
{

constexpr const char* const k_defaultLogPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";

struct LogContext
{
    void UpdateLogger()
    {
        if (!spdlog::thread_pool())
        {
            spdlog::init_thread_pool(8192, 1);
        }

        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(sinkMap.size());
        for (const spdlog::sink_ptr& sink: sinkMap | std::views::values)
        {
            sinks.push_back(sink);
        }

        auto newLogger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        newLogger->set_level(globalLevel);
        newLogger->set_pattern(globalPattern);
        spdlog::set_default_logger(std::move(newLogger));
    }

    HashMap<std::string, spdlog::sink_ptr> sinkMap;
    std::string                            globalPattern = k_defaultLogPattern;
    spdlog::level::level_enum              globalLevel   = spdlog::level::trace;
} g_ctx;

void Init()
{
    TimeStamp   time     = TimeStamp::Current();
    std::string filename = std::format(
        "logs/{:04}-{:02}-{:02}/{:02}h {:02}m {:02}.{:03}s.txt",
        time.year,
        time.month,
        time.day,
        time.hour,
        time.minute,
        time.second,
        time.millisecond);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);

    AddSink(k_consoleSinkName, consoleSink);
    AddSink(k_fileSinkName, fileSink);
    JAM_LOGGER_DEBUG("Logger initialized successfully.");
}

void Shutdown()
{
    JAM_LOGGER_DEBUG("Logger shutting down...");
    spdlog::default_logger()->flush();
    g_ctx.sinkMap.clear();
    spdlog::shutdown();
}

void AddSink(std::string_view _sinkName, const std::shared_ptr<spdlog::sinks::sink>& _sink)
{
    JAM_ASSERT(!g_ctx.sinkMap.contains(std::string(_sinkName)), "Sink already exists: {}", _sinkName);
    g_ctx.sinkMap.emplace(std::string(_sinkName), _sink);
    g_ctx.UpdateLogger();
}

void RemoveSink(std::string_view _sinkName)
{
    auto it = g_ctx.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_ctx.sinkMap.end(), "Sink not found: {}", _sinkName);
    g_ctx.sinkMap.erase(it);
    g_ctx.UpdateLogger();
}

const std::shared_ptr<spdlog::sinks::sink>& GetSink(std::string_view _sinkName)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_ctx.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    return g_ctx.sinkMap[key];
}

void SetLogLevel(eLogLevel _level)
{
    g_ctx.globalLevel = static_cast<spdlog::level::level_enum>(_level);
    spdlog::set_level(g_ctx.globalLevel);
}

void SetLogLevel(std::string_view _sinkName, eLogLevel _level)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_ctx.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    g_ctx.sinkMap[key]->set_level(static_cast<spdlog::level::level_enum>(_level));
}

void SetLogPattern(std::string_view _sinkName, const std::string_view _pattern)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_ctx.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    g_ctx.sinkMap[key]->set_pattern(std::string(_pattern));
}

void SetLogPattern(const std::string_view _pattern)
{
    g_ctx.globalPattern = std::string(_pattern);
    spdlog::set_pattern(g_ctx.globalPattern);
}

void Flush()
{
    if (const auto& logger = spdlog::default_logger())
    {
        logger->flush();
    }
}

}   // namespace jam::logger
