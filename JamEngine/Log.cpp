#include "pch.h"

#include "Log.h"
#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace jam
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
} g_logContext;
}   // namespace jam

namespace jam
{

void Log::Init()
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
    JAM_DEBUG_LOG("log initialized successfully.");
}

void Log::Shutdown()
{
    JAM_DEBUG_LOG("log shutdown.");
    g_logContext.sinkMap.clear();
    spdlog::shutdown();
}

void Log::AddSink(std::string_view _sinkName, const std::shared_ptr<spdlog::sinks::sink>& _sink)
{
    JAM_ASSERT(!g_logContext.sinkMap.contains(std::string(_sinkName)), "Sink already exists: {}", _sinkName);
    g_logContext.sinkMap.emplace(std::string(_sinkName), _sink);
    g_logContext.UpdateLogger();
}

void Log::RemoveSink(std::string_view _sinkName)
{
    auto it = g_logContext.sinkMap.find(std::string(_sinkName));
    JAM_ASSERT(it != g_logContext.sinkMap.end(), "Sink not found: {}", _sinkName);
    g_logContext.sinkMap.erase(it);
    g_logContext.UpdateLogger();
}

const std::shared_ptr<spdlog::sinks::sink>& Log::GetSink(std::string_view _sinkName)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_logContext.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    return g_logContext.sinkMap[key];
}

void Log::SetLogLevel(eLogLevel _level)
{
    g_logContext.globalLevel = static_cast<spdlog::level::level_enum>(_level);
    spdlog::set_level(g_logContext.globalLevel);
}

void Log::SetLogLevel(std::string_view _sinkName, eLogLevel _level)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_logContext.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    g_logContext.sinkMap[key]->set_level(static_cast<spdlog::level::level_enum>(_level));
}

void Log::SetLogPattern(std::string_view _sinkName, const std::string_view _pattern)
{
    std::string key { _sinkName };
    JAM_ASSERT(g_logContext.sinkMap.contains(key), "Sink not found: {}", _sinkName);
    g_logContext.sinkMap[key]->set_pattern(std::string(_pattern));
}

void Log::SetLogPattern(const std::string_view _pattern)
{
    g_logContext.globalPattern = std::string(_pattern);
    spdlog::set_pattern(g_logContext.globalPattern);
}

void Log::Flush()
{
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l)
    {
        l->flush();
    });   
}

}   // namespace jam
