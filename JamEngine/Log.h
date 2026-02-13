#pragma once
#include <spdlog/spdlog.h>

#include "StringHashMap.h"

namespace Jam
{

enum class eLogLevel
{
    Trace    = spdlog::level::trace,
    Debug    = spdlog::level::debug,
    Info     = spdlog::level::info,
    Warn     = spdlog::level::warn,
    Error    = spdlog::level::err,
    Critical = spdlog::level::critical,
    Off      = spdlog::level::off
};

using SinkRef   = std::shared_ptr<spdlog::sinks::sink>;
using LoggerRef = std::shared_ptr<spdlog::logger>;

class LogChannel : public RefCounted
{
public:
    explicit LogChannel(const LoggerRef& _pLogger);

    void AddSink(StringView _name, const SinkRef& _pSink);
    void RemoveSink(StringView _name);
    void SetPattern(StringView _pattern) const;
    void SetLevel(eLogLevel _level) const;

    [[nodiscard]] const SinkRef&   GetSink(StringView _name) const;
    [[nodiscard]] const LoggerRef& GetLogger() const;

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _format, std::forward<Args>(args)...);
    }

    void Log(const eLogLevel _level, const StringView _msg) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _msg.ToStdStringView());
    }

    void Flush() const
    {
        m_pLogger->flush();
    }

private:
    LoggerRef              m_pLogger = nullptr;
    StringHashMap<SinkRef> m_sinkMap = {};
};

class Log
{
public:
    Log(const Log&)            = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&)                 = delete;
    Log& operator=(Log&&)      = delete;

    [[nodiscard]] static Log& GetInstance();

    [[nodiscard]] const Ref<LogChannel>& GetCoreChannel();
    void                                 SetCoreChannel(const Ref<LogChannel>& _pChannel);

    template<typename... Args>
    void Trace(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Trace, _format, std::forward<Args>(args)...);
    }

    void Trace(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Trace, _msg);
    }

    template<typename... Args>
    void Trace(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Trace, _format, std::forward<Args>(args)...);
    }

    void Trace(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Trace, _msg);
    }

    template<typename... Args>
    void Debug(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Debug, _format, std::forward<Args>(args)...);
    }

    void Debug(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Debug, _msg);
    }

    template<typename... Args>
    void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Debug, _format, std::forward<Args>(args)...);
    }

    void Debug(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Debug, _msg);
    }

    template<typename... Args>
    void Info(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Info, _format, std::forward<Args>(args)...);
    }

    void Info(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Info, _msg);
    }

    template<typename... Args>
    void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Info, _format, std::forward<Args>(args)...);
    }

    void Info(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Info, _msg);
    }

    template<typename... Args>
    void Warn(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Warn, _format, std::forward<Args>(args)...);
    }

    void Warn(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Warn, _msg);
    }

    template<typename... Args>
    void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Warn, _format, std::forward<Args>(args)...);
    }

    void Warn(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Warn, _msg);
    }

    template<typename... Args>
    void Error(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Error, _format, std::forward<Args>(args)...);
    }

    void Error(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Error, _msg);
    }

    template<typename... Args>
    void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Error, _format, std::forward<Args>(args)...);
    }

    void Error(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Error, _msg);
    }

    template<typename... Args>
    void Critical(const Ref<LogChannel>& _pChannel, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(eLogLevel::Critical, _format, std::forward<Args>(args)...);
    }

    void Critical(const Ref<LogChannel>& _pChannel, const StringView _msg) const
    {
        _pChannel->Log(eLogLevel::Critical, _msg);
    }

    template<typename... Args>
    void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        m_pCoreLogger->Log(eLogLevel::Critical, _format, std::forward<Args>(args)...);
    }

    void Critical(const StringView _msg) const
    {
        m_pCoreLogger->Log(eLogLevel::Critical, _msg);
    }

    void Flush() const;
    void Flush(const Ref<LogChannel>& _pChannel) const;

private:
    Log();
    ~Log();

    static void CreateInstance();
    static void DestroyInstance();

    Ref<LogChannel> m_pCoreLogger = nullptr;

    inline static Log* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_LOG Jam::Log::GetInstance()

#ifdef JAM_DEBUG
#    define JAM_DEBUG_LOG(...) JAM_LOG.Debug(__VA_ARGS__)
#else
#    define JAM_DEBUG_LOG(...) JAM_NOOP
#endif