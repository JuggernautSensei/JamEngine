#pragma once
#include <spdlog/spdlog.h>

#include "StringKeyContainers.h"

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
    void Log(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _format, std::forward<Args>(args)...);
    }

    void Log(const eLogLevel _level, const StringView _msg) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, { _msg.GetPtr(), NumericCast<size_t>(_msg.GetLength()) });
    }

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(const eLogLevel _level, const WStringView _msg) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, { _msg.GetPtr(), NumericCast<size_t>(_msg.GetLength()) });
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

    [[nodiscard]] const Ref<LogChannel>& GetCoreChannel() const;
    void                                 SetCoreChannel(const Ref<LogChannel>& _pChannel);

    template<typename... Args>
    void Logging(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        GetCoreChannel()->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Logging(const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args)
    {
        GetCoreChannel()->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Logging(const Ref<LogChannel>& _pChannel, const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Logging(const Ref<LogChannel>& _pChannel, const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(_level, _format, std::forward<Args>(args)...);
    }

    void Logging(const eLogLevel _level, const StringView _msg) const
    {
        GetCoreChannel()->Log(_level, _msg);
    }

    void Logging(const eLogLevel _level, const WStringView _msg) const
    {
        GetCoreChannel()->Log(_level, _msg);
    }

    void Logging(const Ref<LogChannel>& _pChannel, const eLogLevel _level, const StringView _msg) const
    {
        _pChannel->Log(_level, _msg);
    }

    void Logging(const Ref<LogChannel>& _pChannel, const eLogLevel _level, const WStringView _msg) const
    {
        _pChannel->Log(_level, _msg);
    }

    void Flush() const;
    void Flush(const Ref<LogChannel>& _pChannel) const;

    static void CreateInstance();
    static void DestroyInstance();

private:
    Log();
    ~Log();

    Ref<LogChannel> m_pCoreLogger = nullptr;

    inline static Log* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_LOG Jam::Log::GetInstance()

#define JAM_LOG_TRACE(...)    JAM_LOG.Logging(Jam::eLogLevel::Trace, __VA_ARGS__)
#define JAM_LOG_INFO(...)     JAM_LOG.Logging(Jam::eLogLevel::Info, __VA_ARGS__)
#define JAM_LOG_WARN(...)     JAM_LOG.Logging(Jam::eLogLevel::Warn, __VA_ARGS__)
#define JAM_LOG_ERROR(...)    JAM_LOG.Logging(Jam::eLogLevel::Error, __VA_ARGS__)
#define JAM_LOG_CRITICAL(...) JAM_LOG.Logging(Jam::eLogLevel::Critical, __VA_ARGS__)

#define JAM_LOG_CHANNEL_TRACE(_channel, ...)    JAM_LOG.Logging(_channel, Jam::eLogLevel::Trace, __VA_ARGS__)
#define JAM_LOG_CHANNEL_INFO(_channel, ...)     JAM_LOG.Logging(_channel, Jam::eLogLevel::Info, __VA_ARGS__)
#define JAM_LOG_CHANNEL_WARN(_channel, ...)     JAM_LOG.Logging(_channel, Jam::eLogLevel::Warn, __VA_ARGS__)
#define JAM_LOG_CHANNEL_ERROR(_channel, ...)    JAM_LOG.Logging(_channel, Jam::eLogLevel::Error, __VA_ARGS__)
#define JAM_LOG_CHANNEL_CRITICAL(_channel, ...) JAM_LOG.Logging(_channel, Jam::eLogLevel::Critical, __VA_ARGS__)

#ifdef JAM_DEBUG
#    define JAM_LOG_DEBUG(...)                   JAM_LOG.Logging(Jam::eLogLevel::Debug, __VA_ARGS__)
#    define JAM_LOG_CHANNEL_DEBUG(_channel, ...) JAM_LOG.Logging(_channel, Jam::eLogLevel::Debug, __VA_ARGS__)
#else
#    define JAM_LOG_DEBUG(...)                   JAM_NOOP
#    define JAM_LOG_CHANNEL_DEBUG(_channel, ...) JAM_NOOP
#endif