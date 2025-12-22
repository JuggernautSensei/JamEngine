#include "pch.h"

#include "Log.h"
#include "Timer.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace jam
{

struct LogContext
{
    void UpdateLogger() const
    {
        // create thread pool if nee
        if (!spdlog::thread_pool())
        {
            spdlog::init_thread_pool(8192, 1);
        }

        // make new logger
        Vector<SinkPtr> sinks;
        sinks.reserve(sinks.size());
        for (const SinkPtr& sink: this->sinks | std::views::values)
        {
            sinks.emplace_back(sink);
        }

        std::shared_ptr<spdlog::async_logger> pLogger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        pLogger->set_level(globalLevel);
        pLogger->set_pattern(globalPattern);
        spdlog::set_default_logger(std::move(pLogger));
    }

    HashMap<std::string, SinkPtr> sinks        = {};
    SinkPtr                       pConsoleSink = nullptr;
    SinkPtr                       pFileSink    = nullptr;

    std::string               globalPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";
    spdlog::level::level_enum globalLevel   = spdlog::level::trace;

}* g_pLogCtx = nullptr;

}   // namespace jam

namespace jam
{

void Log::Init()
{
    JAM_ASSERT(g_pLogCtx == nullptr, "Log is already initialized.");
    g_pLogCtx = JAM_NEW(LogContext);

    const TimeStamp   ts       = TimeStamp::Current();
    const std::string filename = std::format("Logs/{:04}-{:02}-{:02}/{:02}h {:02}m {:02}.{:03}s.log", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);

    g_pLogCtx->pConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    g_pLogCtx->pFileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);

    AddSink("_JAM_CONSOLE_SINK_", g_pLogCtx->pConsoleSink);
    AddSink("_JAM_FILE_SINK_", g_pLogCtx->pFileSink);
    JAM_DEBUG_LOG("log initialized successfully.");
}

void Log::Shutdown()
{
    JAM_ASSERT(g_pLogCtx, "Log is not initialized.");

    JAM_DEBUG_LOG("log shutdown.");
    spdlog::shutdown();

    JAM_DELETE(g_pLogCtx);
}

void Log::AddSink(
    StringView     _name,
    const SinkPtr& _pSink)
{
    const std::string name  = _name.ToString();
    const auto [_, bResult] = g_pLogCtx->sinks.emplace(name, _pSink);
    JAM_ASSERT(bResult, "Sink already exists: {}", _name);
    g_pLogCtx->UpdateLogger();
}

void Log::RemoveSink(
    StringView _name)
{
    const std::string name = _name.ToString();
    JAM_ASSERT(g_pLogCtx->sinks.contains(name), "Sink not found: {}", _name);
    g_pLogCtx->sinks.erase(name);
    g_pLogCtx->UpdateLogger();
}

const SinkPtr& Log::GetSink(
    StringView _name)
{
    const std::string name = _name.ToString();
    JAM_ASSERT(g_pLogCtx->sinks.contains(name), "Sink not found: {}", _name);
    return g_pLogCtx->sinks[name];
}

const SinkPtr& Log::GetConsoleSink()
{
    return g_pLogCtx->pConsoleSink;
}

const SinkPtr& Log::GetFileSink()
{
    return g_pLogCtx->pFileSink;
}

void Log::SetLogLevel(
    eLogLevel _level)
{
    g_pLogCtx->globalLevel = static_cast<spdlog::level::level_enum>(_level);
    spdlog::set_level(g_pLogCtx->globalLevel);
}

void Log::SetLogLevel(
    const StringView _name,
    eLogLevel        _level)
{
    const std::string name  = _name.ToString();
    const SinkPtr     pSink = GetSink(_name);
    pSink->set_level(static_cast<spdlog::level::level_enum>(_level));
}

void Log::SetLogPattern(
    const StringView _name,
    const StringView _pattern)
{
    const std::string name    = _name.ToString();
    const std::string pattern = _pattern.ToString();
    const SinkPtr     pSink   = GetSink(_name);
    pSink->set_pattern(pattern);
}

void Log::SetLogPattern(
    const StringView _pattern)
{
    g_pLogCtx->globalPattern = _pattern.ToString();
    spdlog::set_pattern(g_pLogCtx->globalPattern);
}

}   // namespace jam
