#include "pch.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "LogSystem.h"
#include "Timer.h"
#include "String.h"

// log channel
namespace Jam
{

LogChannel::LogChannel(const LoggerRef& _pLogger)
    : m_pLogger(_pLogger)
{
    JAM_ASSERT(_pLogger != nullptr, "Logger cannot be null.");
}

void LogChannel::AddSink(const StringView _name, const SinkRef& _pSink)
{
    const String name   = _name;
    const auto [_, bOk] = m_sinkMap.emplace(name, _pSink);
    JAM_ASSERT(bOk, "Sink with name '{}' already exists.", name);
    m_pLogger->sinks().emplace_back(_pSink);
}

void LogChannel::RemoveSink(const StringView _name)
{
    const auto it = m_sinkMap.find(_name);
    JAM_ASSERT(it != m_sinkMap.end(), "Sink with name '{}' does not exist.", _name);
    std::erase(m_pLogger->sinks(), it->second);
    m_sinkMap.erase(it);
}

void LogChannel::SetPattern(const StringView _pattern) const
{
    m_pLogger->set_pattern({ _pattern.GetPtr(), static_cast<size_t>(_pattern.GetLength()) });
}

void LogChannel::SetLevel(const eLogLevel _level) const
{
    spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
    m_pLogger->set_level(level);
}

const SinkRef& LogChannel::GetSink(const StringView _name) const
{
    const auto it = m_sinkMap.find(_name);
    JAM_ASSERT(it != m_sinkMap.end(), "Sink with name '{}' does not exist.", _name);
    return it->second;
}

const LoggerRef& LogChannel::GetLogger() const
{
    return m_pLogger;
}

}   // namespace Jam

namespace Jam
{

LogSystem& LogSystem::GetInstance()
{
    JAM_ASSERT(s_pInstance != nullptr, "Log is not initialized.");
    return *s_pInstance;
}

const Ref<LogChannel>& LogSystem::GetCoreChannel() const
{
    JAM_ASSERT(m_pCoreLogger != nullptr, "Log is not initialized.");
    return m_pCoreLogger;
}

void LogSystem::SetCoreChannel(
    const Ref<LogChannel>& _pChannel)
{
    JAM_ASSERT(_pChannel != nullptr, "Log channel cannot be null.");
    m_pCoreLogger = _pChannel;
    spdlog::set_default_logger(_pChannel->GetLogger());
}

void LogSystem::Flush() const
{
    m_pCoreLogger->Flush();
}

void LogSystem::Flush(const Ref<LogChannel>& _pChannel) const
{
    _pChannel->Flush();
}

void LogSystem::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "Log instance already exists.");
    s_pInstance = new LogSystem;
}

LogSystem::LogSystem()
{
    // console sink
    const SinkRef pConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // file sink
    const TimeStamp ts   = TimeStamp::Current();
    String          path = {};
    path.Format("Logs/{:04}.{:02}.{:02}/{:02}h {:02}m {:02}.{:03}s.log", ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);
    const SinkRef pFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.GetPtr(), true);

    // core logger
    spdlog::init_thread_pool(8192, 1);
    const LoggerRef pCoreLogger = std::make_shared<spdlog::async_logger>("CORE", spdlog::sinks_init_list { pConsoleSink, pFileSink }, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    pCoreLogger->set_level(spdlog::level::trace);
    SetCoreChannel(MakeRef<LogChannel>(pCoreLogger));
}

LogSystem::~LogSystem()
{
    JAM_ASSERT(m_pCoreLogger != nullptr, "Log is not initialized.");
    spdlog::shutdown();
    m_pCoreLogger.Reset();
}

void LogSystem::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

}   // namespace Jam
